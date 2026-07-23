#include "game/CombatContext.hpp"

#include "common/BoardDimensions.hpp"
#include "common/PlayerArea.hpp"
#include "game/BoxBoard.hpp"
#include "game/CombatRandom.hpp"
#include "game/Hero.hpp"
#include "game/Tower.hpp"

#include <stdexcept>
#include <utility>

namespace sts {
namespace {

CombatEvent baseEvent(CombatEventType type,
                      HeroId attackerId,
                      std::optional<HeroId> targetId,
                      int actingPlayerId,
                      int targetPlayerId) {
    CombatEvent event;
    event.type = type;
    event.attackerHeroId = attackerId;
    event.targetHeroId = targetId;
    event.actingPlayerId = actingPlayerId;
    event.targetPlayerId = targetPlayerId;
    return event;
}

void copyDamage(CombatEvent& event, const DamageResult& result, DamageKind kind) {
    event.damageKind = kind;
    event.baseDamage = result.baseDamage;
    event.damageAfterWeak = result.damageAfterWeak;
    event.damageAfterVulnerable = result.damageAfterVulnerable;
    event.damageAfterDefense = result.damageAfterDefense;
    event.shieldAbsorbed = result.shieldAbsorbed;
    event.damage = result.hpDamage;
    event.remainingShield = result.remainingShield;
    event.remainingHp = result.remainingHp;
    event.calculatedDamage = result.calculatedDamage;
    event.hpDamageApplied = result.hpDamageApplied;
    event.overkillDamage = result.overkillDamage;
    event.targetRemainingHp = result.targetRemainingHp;
}

} // namespace

CombatContext::CombatContext(int actingPlayerId,
                             BoxBoard& board,
                             std::array<Tower, 2>& towers,
                             CombatRandom& random,
                             std::vector<CombatEvent>& events)
    : actingPlayerId_(actingPlayerId), board_(board), towers_(towers), random_(random), events_(events) {
    if (actingPlayerId != 0 && actingPlayerId != 1) {
        throw std::invalid_argument("actingPlayerId must be 0 or 1");
    }
}

std::vector<HeroId> CombatContext::livingEnemyHeroIds() const {
    std::vector<HeroId> ids;
    const int playerId = defendingPlayerId();
    if (playerId == 1) {
        for (int row = PlayerAreaRows - 1; row >= 0; --row) {
            for (int column = 0; column < BoardColumns; ++column) {
                const auto* hero = dynamic_cast<const Hero*>(board_.pieceAt({row, column}));
                if (hero != nullptr && hero->isAlive()) {
                    ids.push_back(hero->id());
                }
            }
        }
    } else {
        for (int row = PlayerAreaRows; row < BoardRows; ++row) {
            for (int column = 0; column < BoardColumns; ++column) {
                const auto* hero = dynamic_cast<const Hero*>(board_.pieceAt({row, column}));
                if (hero != nullptr && hero->isAlive()) {
                    ids.push_back(hero->id());
                }
            }
        }
    }
    return ids;
}

bool CombatContext::hasLivingEnemies() const {
    return !livingEnemyHeroIds().empty();
}

std::optional<HeroId> CombatContext::firstEnemyInNormalAttackOrder() const {
    const std::vector<HeroId> ids = livingEnemyHeroIds();
    return ids.empty() ? std::nullopt : std::optional<HeroId>{ids.front()};
}

Hero* CombatContext::findHero(HeroId id) noexcept {
    return board_.heroById(id);
}

const Hero* CombatContext::findHero(HeroId id) const noexcept {
    return board_.heroById(id);
}

DamageResult CombatContext::damageHero(HeroId attackerId,
                                       HeroId targetId,
                                       int baseDamage,
                                       DamageKind kind) {
    Hero* attacker = findHero(attackerId);
    Hero* target = findHero(targetId);
    if (attacker == nullptr || target == nullptr || !isValidLivingAttacker(attackerId) ||
        !isValidLivingEnemy(targetId)) {
        return {};
    }

    CombatEvent attacked = baseEvent(CombatEventType::HeroAttacked, attackerId, targetId,
                                     actingPlayerId_, defendingPlayerId());
    attacked.damageKind = kind;
    attacked.baseDamage = baseDamage;
    attacked.remainingHp = target->currentHp();
    attacked.remainingShield = target->shield();
    emitEvent(std::move(attacked));

    const DamageResult result = damageResolver_.resolveHeroDamage(*attacker, *target, baseDamage, kind);
    if (result.shieldAbsorbed > 0) {
        CombatEvent absorbed = baseEvent(CombatEventType::ShieldAbsorbed, attackerId, targetId,
                                         actingPlayerId_, defendingPlayerId());
        copyDamage(absorbed, result, kind);
        absorbed.amount = result.shieldAbsorbed;
        emitEvent(std::move(absorbed));
        if (result.remainingShield == 0) {
            CombatEvent broken = baseEvent(CombatEventType::ShieldBroken, attackerId, targetId,
                                           actingPlayerId_, defendingPlayerId());
            copyDamage(broken, result, kind);
            emitEvent(std::move(broken));
        }
    }

    CombatEvent damaged = baseEvent(CombatEventType::HeroDamaged, attackerId, targetId,
                                    actingPlayerId_, defendingPlayerId());
    copyDamage(damaged, result, kind);
    emitEvent(std::move(damaged));
    if (result.targetDied) {
        CombatEvent died = baseEvent(CombatEventType::HeroDied, attackerId, targetId,
                                    actingPlayerId_, defendingPlayerId());
        copyDamage(died, result, kind);
        emitEvent(std::move(died));
    }
    return result;
}

DamageResult CombatContext::damageTower(HeroId attackerId,
                                        int baseDamage,
                                        DamageKind kind) {
    Tower& tower = towers_[static_cast<std::size_t>(defendingPlayerId())];
    Hero* attacker = findHero(attackerId);
    if (attacker == nullptr || !isValidLivingAttacker(attackerId) || tower.isDestroyed() ||
        (kind == DamageKind::NormalAttack && hasLivingEnemies())) {
        return {};
    }

    const DamageResult result = damageResolver_.resolveTowerDamage(*attacker, tower, baseDamage, kind);
    CombatEvent damaged = baseEvent(CombatEventType::TowerDamaged, attackerId, std::nullopt,
                                    actingPlayerId_, defendingPlayerId());
    copyDamage(damaged, result, kind);
    emitEvent(std::move(damaged));
    if (tower.isDestroyed()) {
        CombatEvent destroyed = baseEvent(CombatEventType::TowerDestroyed, attackerId, std::nullopt,
                                          actingPlayerId_, defendingPlayerId());
        copyDamage(destroyed, result, kind);
        destroyed.winnerPlayerId = actingPlayerId_;
        emitEvent(std::move(destroyed));
    }
    return result;
}

LightningTarget CombatContext::chooseRandomLightningTarget() {
    std::vector<LightningTarget> candidates;
    for (const HeroId id : livingEnemyHeroIds()) {
        candidates.push_back({LightningTargetKind::Hero, id, defendingPlayerId()});
    }
    if (!towers_[static_cast<std::size_t>(defendingPlayerId())].isDestroyed()) {
        candidates.push_back({LightningTargetKind::Tower, std::nullopt, defendingPlayerId()});
    }
    if (candidates.empty()) {
        throw std::logic_error("no legal lightning target exists");
    }
    return candidates[random_.chooseIndex(candidates.size())];
}

void CombatContext::addVulnerable(HeroId attackerId, HeroId targetId, int layers) {
    Hero* target = findHero(targetId);
    if (!isValidLivingAttacker(attackerId) || target == nullptr || !isValidLivingEnemy(targetId) || layers <= 0) {
        return;
    }
    const int previousLayers = target->vulnerableLayers();
    target->addVulnerableLayers(layers);
    CombatEvent event = baseEvent(CombatEventType::VulnerableApplied, attackerId, targetId,
                                  actingPlayerId_, defendingPlayerId());
    event.amount = layers;
    event.vulnerableLayers = target->vulnerableLayers();
    event.previousLayers = previousLayers;
    event.addedLayers = layers;
    event.totalLayers = target->vulnerableLayers();
    emitEvent(std::move(event));
}

void CombatContext::addWeak(HeroId attackerId, HeroId targetId, int layers) {
    Hero* target = findHero(targetId);
    if (!isValidLivingAttacker(attackerId) || target == nullptr || !isValidLivingEnemy(targetId) || layers <= 0) {
        return;
    }
    const int previousLayers = target->weakLayers();
    target->addWeakLayers(layers);
    CombatEvent event = baseEvent(CombatEventType::WeakApplied, attackerId, targetId,
                                  actingPlayerId_, defendingPlayerId());
    event.amount = layers;
    event.weakLayers = target->weakLayers();
    event.previousLayers = previousLayers;
    event.addedLayers = layers;
    event.totalLayers = target->weakLayers();
    emitEvent(std::move(event));
}

int CombatContext::healHero(HeroId heroId, int amount) {
    Hero* hero = findHero(heroId);
    if (hero == nullptr || !isValidLivingAttacker(heroId)) {
        return 0;
    }
    const int healed = hero->heal(amount);
    if (healed > 0) {
        CombatEvent event = baseEvent(CombatEventType::HeroHealed, heroId, heroId,
                                      actingPlayerId_, actingPlayerId_);
        event.amount = healed;
        event.remainingHp = hero->currentHp();
        emitEvent(std::move(event));
    }
    return healed;
}

int CombatContext::gainShield(HeroId heroId, int amount) {
    Hero* hero = findHero(heroId);
    if (hero == nullptr || !isValidLivingAttacker(heroId)) {
        return 0;
    }
    const int gained = hero->gainShield(amount);
    if (gained > 0) {
        CombatEvent event = baseEvent(CombatEventType::ShieldGained, heroId, heroId,
                                      actingPlayerId_, actingPlayerId_);
        event.amount = gained;
        event.remainingShield = hero->shield();
        emitEvent(std::move(event));
    }
    return gained;
}

void CombatContext::emitEvent(CombatEvent event) {
    events_.push_back(std::move(event));
}

bool CombatContext::gameFinished() const noexcept {
    return towers_[static_cast<std::size_t>(defendingPlayerId())].isDestroyed();
}

bool CombatContext::isValidLivingAttacker(HeroId attackerId) const noexcept {
    const Hero* attacker = findHero(attackerId);
    return attacker != nullptr && attacker->isAlive() &&
           playerIdForPosition(attacker->position()) == actingPlayerId_;
}

bool CombatContext::isValidLivingEnemy(HeroId targetId) const noexcept {
    const Hero* target = findHero(targetId);
    return target != nullptr && target->isAlive() &&
           playerIdForPosition(target->position()) == defendingPlayerId();
}

} // namespace sts
