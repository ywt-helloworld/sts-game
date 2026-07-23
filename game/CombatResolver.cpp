#include "game/CombatResolver.hpp"

#include "common/BoardDimensions.hpp"
#include "common/PlayerArea.hpp"
#include "game/BoxBoard.hpp"
#include "game/CombatContext.hpp"
#include "game/Hero.hpp"
#include "game/Tower.hpp"

#include <stdexcept>

namespace sts {
namespace {

CombatEvent phaseEvent(CombatEventType type,
                       int actingPlayerId,
                       int defendingPlayerId,
                       std::optional<int> winner = std::nullopt) {
    CombatEvent event;
    event.type = type;
    event.actingPlayerId = actingPlayerId;
    event.targetPlayerId = defendingPlayerId;
    event.winnerPlayerId = winner;
    return event;
}

} // namespace

CombatResolver::CombatResolver(BoxBoard& board, std::array<Tower, 2>& towers) noexcept
    : board_(board), towers_(towers), random_(&ownedRandom_) {}

CombatResolver::CombatResolver(BoxBoard& board,
                               std::array<Tower, 2>& towers,
                               CombatRandom& random) noexcept
    : board_(board), towers_(towers), random_(&random) {}

std::vector<HeroId> CombatResolver::collectAttackers(int actingPlayerId) const {
    if (actingPlayerId != 0 && actingPlayerId != 1) {
        throw std::invalid_argument("actingPlayerId must be 0 or 1");
    }

    std::vector<HeroId> ids;
    const auto appendLivingHero = [this, &ids, actingPlayerId](Position position) {
        const auto* hero = dynamic_cast<const Hero*>(board_.pieceAt(position));
        if (hero != nullptr && hero->isAlive() &&
            playerIdForPosition(hero->position()) == actingPlayerId) {
            ids.push_back(hero->id());
        }
    };

    if (actingPlayerId == 1) {
        for (int row = 0; row < PlayerAreaRows; ++row) {
            for (int column = 0; column < BoardColumns; ++column) {
                appendLivingHero({row, column});
            }
        }
    } else {
        for (int row = BoardRows - 1; row >= PlayerAreaRows; --row) {
            for (int column = BoardColumns - 1; column >= 0; --column) {
                appendLivingHero({row, column});
            }
        }
    }
    return ids;
}

CombatResolution CombatResolver::resolve(int actingPlayerId) {
    if (actingPlayerId != 0 && actingPlayerId != 1) {
        throw std::invalid_argument("actingPlayerId must be 0 or 1");
    }

    CombatResolution resolution;
    const int defendingPlayerId = 1 - actingPlayerId;
    resolution.events.push_back(phaseEvent(CombatEventType::CombatStarted,
                                           actingPlayerId, defendingPlayerId));
    CombatContext context(actingPlayerId, board_, towers_, *random_, resolution.events);
    const std::vector<HeroId> attackerIds = collectAttackers(actingPlayerId);
    for (const HeroId attackerId : attackerIds) {
        if (context.gameFinished()) {
            resolution.gameFinished = true;
            resolution.winnerPlayerId = actingPlayerId;
            break;
        }

        Hero* attacker = board_.heroById(attackerId);
        if (attacker == nullptr || !attacker->isAlive() ||
            playerIdForPosition(attacker->position()) != actingPlayerId) {
            continue;
        }

        attacker->performAttack(context);
        resolveDeathsAndBoardChanges(resolution.events, actingPlayerId);
        if (context.gameFinished()) {
            resolution.gameFinished = true;
            resolution.winnerPlayerId = actingPlayerId;
            break;
        }
    }

    processTurnEndStatuses(resolution.events, actingPlayerId);
    resolveDeathsAndBoardChanges(resolution.events, actingPlayerId);
    resolution.events.push_back(phaseEvent(CombatEventType::CombatFinished,
                                           actingPlayerId, defendingPlayerId,
                                           resolution.winnerPlayerId));
    if (resolution.gameFinished) {
        resolution.events.push_back(phaseEvent(CombatEventType::GameFinished,
                                               actingPlayerId, defendingPlayerId,
                                               resolution.winnerPlayerId));
    }
    return resolution;
}

void CombatResolver::resolveDeathsAndBoardChanges(std::vector<CombatEvent>& events,
                                                  int actingPlayerId) {
    for (const DeadHeroInfo& dead : board_.convertDeadHeroesToBoxes()) {
        CombatEvent converted;
        converted.type = CombatEventType::HeroConvertedToBox;
        converted.targetHeroId = dead.id;
        converted.actingPlayerId = actingPlayerId;
        converted.targetPlayerId = playerIdForPosition(dead.position);
        events.push_back(std::move(converted));
    }
}

void CombatResolver::processTurnEndStatuses(std::vector<CombatEvent>& events, int actingPlayerId) {
    for (const HeroId heroId : board_.livingHeroIds()) {
        Hero* hero = board_.heroById(heroId);
        if (hero == nullptr) {
            continue;
        }
        const int oldVulnerable = hero->vulnerableLayers();
        const int oldWeak = hero->weakLayers();
        hero->processTurnEndStatuses();
        const std::optional<int> heroPlayerId = playerIdForPosition(hero->position());

        if (oldVulnerable > 0) {
            CombatEvent event;
            event.type = hero->vulnerableLayers() == 0
                             ? CombatEventType::VulnerableExpired
                             : CombatEventType::VulnerableReduced;
            event.targetHeroId = heroId;
            event.actingPlayerId = actingPlayerId;
            event.targetPlayerId = heroPlayerId;
            event.amount = -1;
            event.vulnerableLayers = hero->vulnerableLayers();
            events.push_back(std::move(event));
        }
        if (oldWeak > 0) {
            CombatEvent event;
            event.type = hero->weakLayers() == 0
                             ? CombatEventType::WeakExpired
                             : CombatEventType::WeakReduced;
            event.targetHeroId = heroId;
            event.actingPlayerId = actingPlayerId;
            event.targetPlayerId = heroPlayerId;
            event.amount = -1;
            event.weakLayers = hero->weakLayers();
            events.push_back(std::move(event));
        }
    }

    for (Tower& tower : towers_) {
        const int oldVulnerable = tower.vulnerableLayers();
        const int oldWeak = tower.weakLayers();
        tower.processTurnEndStatuses();

        if (oldVulnerable > 0) {
            CombatEvent event;
            event.type = tower.vulnerableLayers() == 0
                             ? CombatEventType::VulnerableExpired
                             : CombatEventType::VulnerableReduced;
            event.actingPlayerId = actingPlayerId;
            event.targetPlayerId = tower.playerId();
            event.targetType = CombatTargetType::Tower;
            event.targetTowerPlayerId = tower.playerId();
            event.amount = -1;
            event.previousLayers = oldVulnerable;
            event.vulnerableLayers = tower.vulnerableLayers();
            event.totalLayers = tower.vulnerableLayers();
            events.push_back(std::move(event));
        }

        if (oldWeak > 0) {
            CombatEvent event;
            event.type = tower.weakLayers() == 0
                             ? CombatEventType::WeakExpired
                             : CombatEventType::WeakReduced;
            event.actingPlayerId = actingPlayerId;
            event.targetPlayerId = tower.playerId();
            event.targetType = CombatTargetType::Tower;
            event.targetTowerPlayerId = tower.playerId();
            event.amount = -1;
            event.previousLayers = oldWeak;
            event.weakLayers = tower.weakLayers();
            event.totalLayers = tower.weakLayers();
            events.push_back(std::move(event));
        }
    }
}

} // namespace sts
