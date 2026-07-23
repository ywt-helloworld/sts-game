#include "game/CombatResolver.hpp"

#include "common/BoardDimensions.hpp"
#include "common/PlayerArea.hpp"
#include "game/BoxBoard.hpp"
#include "game/CombatContext.hpp"
#include "game/Hero.hpp"
#include "game/Tower.hpp"

#include <algorithm>
#include <stdexcept>
#include <tuple>

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
    std::vector<HeroId> ids = board_.livingHeroIdsForPlayer(actingPlayerId);
    std::ranges::sort(ids, [this, actingPlayerId](HeroId leftId, HeroId rightId) {
        const Hero* left = board_.heroById(leftId);
        const Hero* right = board_.heroById(rightId);
        if (left == nullptr || right == nullptr) {
            return leftId < rightId;
        }
        const auto orderKey = [actingPlayerId](const Hero& hero) {
            const int distance = actingPlayerId == 0 ? hero.position().row - PlayerAreaRows
                                                     : PlayerAreaRows - 1 - hero.position().row;
            return std::tuple{distance, hero.position().column, hero.id()};
        };
        return orderKey(*left) < orderKey(*right);
    });
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
    const std::vector<HeroId> attackerOrder = collectAttackers(actingPlayerId);
    for (const HeroId attackerId : attackerOrder) {
        Hero* attacker = board_.heroById(attackerId);
        if (attacker == nullptr || !attacker->isAlive() ||
            playerIdForPosition(attacker->position()) != actingPlayerId) {
            continue;
        }
        attacker->performAttack(context);
        if (context.gameFinished()) {
            resolution.gameFinished = true;
            resolution.winnerPlayerId = actingPlayerId;
            break;
        }
    }

    processTurnEndStatuses(resolution.events, actingPlayerId);
    for (const DeadHeroInfo& dead : board_.convertDeadHeroesToBoxes()) {
        CombatEvent converted;
        converted.type = CombatEventType::HeroConvertedToBox;
        converted.targetHeroId = dead.id;
        converted.actingPlayerId = actingPlayerId;
        converted.targetPlayerId = playerIdForPosition(dead.position);
        resolution.events.push_back(std::move(converted));
    }
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
}

} // namespace sts
