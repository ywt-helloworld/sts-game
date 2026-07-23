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
    event.targetRemainingHp = result.targetRemainingHp;
    event.shieldBefore = result.shieldBefore;
    event.shieldAfter = result.shieldAfter;
    event.hpBefore = result.hpBefore;
    event.hpAfter = result.hpAfter;
    event.overflowDamage = result.overflowDamage;
    event.towerHpBefore = result.towerHpBefore;
    event.towerDamageApplied = result.towerDamageApplied;
    event.towerHpAfter = result.towerHpAfter;
    event.towerDestroyed = result.towerDestroyed;
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
    const int defender = defendingPlayerId();
    if (defender == 1) {
        for (int row = PlayerAreaRows - 1; row >= 0; --row) {
            for (int column = BoardColumns - 1; column >= 0; --column) {
                const auto* hero = dynamic_cast<const Hero*>(board_.pieceAt({row, column}));
                if (hero != nullptr && hero->isAlive()) {
                    return hero->id();
                }
            }
        }
    } else {
        for (int row = PlayerAreaRows; row < BoardRows; ++row) {
            for (int column = 0; column < BoardColumns; ++column) {
                const auto* hero = dynamic_cast<const Hero*>(board_.pieceAt({row, column}));
                if (hero != nullptr && hero->isAlive()) {
                    return hero->id();
                }
            }
        }
    }
    return std::nullopt;
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

    DamageResult result = damageResolver_.resolveHeroDamage(*attacker, *target, baseDamage, kind);
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

    if (result.overflowDamage > 0) {
        Tower& tower = towers_[static_cast<std::size_t>(defendingPlayerId())];
        result.towerHpBefore = tower.currentHp();
        result.towerDamageApplied = tower.takeDamage(result.overflowDamage);
        result.towerHpAfter = tower.currentHp();
        result.towerDestroyed = tower.isDestroyed();

        CombatEvent overflow = baseEvent(CombatEventType::OverflowDamageGenerated,
                                         attackerId, targetId,
                                         actingPlayerId_, defendingPlayerId());
        copyDamage(overflow, result, kind);
        overflow.amount = result.overflowDamage;
        overflow.towerDamageSource = TowerDamageSource::Overflow;
        emitEvent(std::move(overflow));

        CombatEvent towerDamaged = baseEvent(CombatEventType::TowerDamaged,
                                             attackerId, std::nullopt,
                                             actingPlayerId_, defendingPlayerId());
        copyDamage(towerDamaged, result, kind);
        towerDamaged.towerDamageSource = TowerDamageSource::Overflow;
        towerDamaged.damage = result.towerDamageApplied;
        towerDamaged.remainingHp = result.towerHpAfter;
        towerDamaged.targetRemainingHp = result.towerHpAfter;
        emitEvent(std::move(towerDamaged));

        if (result.towerDestroyed) {
            CombatEvent destroyed = baseEvent(CombatEventType::TowerDestroyed,
                                              attackerId, std::nullopt,
                                              actingPlayerId_, defendingPlayerId());
            copyDamage(destroyed, result, kind);
            destroyed.towerDamageSource = TowerDamageSource::Overflow;
            destroyed.damage = result.towerDamageApplied;
            destroyed.remainingHp = result.towerHpAfter;
            destroyed.targetRemainingHp = result.towerHpAfter;
            destroyed.winnerPlayerId = actingPlayerId_;
            emitEvent(std::move(destroyed));
        }
    }

    if (result.targetDied) {
        convertDeadHeroesToBoxes();
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
    damaged.towerDamageSource = TowerDamageSource::DirectAttack;
    damaged.damage = result.towerDamageApplied;
    damaged.remainingHp = result.towerHpAfter;
    damaged.targetRemainingHp = result.towerHpAfter;
    emitEvent(std::move(damaged));
    if (tower.isDestroyed()) {
        CombatEvent destroyed = baseEvent(CombatEventType::TowerDestroyed, attackerId, std::nullopt,
                                          actingPlayerId_, defendingPlayerId());
        copyDamage(destroyed, result, kind);
        destroyed.towerDamageSource = TowerDamageSource::DirectAttack;
        destroyed.damage = result.towerDamageApplied;
        destroyed.remainingHp = result.towerHpAfter;
        destroyed.targetRemainingHp = result.towerHpAfter;
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

void CombatContext::applyDebuffAfterHeroHit(HeroId sourceHeroId,
                                            HeroId originalTargetHeroId,
                                            const DamageResult& damageResult,
                                            const TargetDebuff& debuff) {
    if (!isValidLivingAttacker(sourceHeroId) ||
        (debuff.vulnerableLayers <= 0 && debuff.weakLayers <= 0)) {
        return;
    }

    if (!damageResult.targetDied) {
        if (debuff.vulnerableLayers > 0) {
            addVulnerable(sourceHeroId, originalTargetHeroId, debuff.vulnerableLayers);
        }
        if (debuff.weakLayers > 0) {
            addWeak(sourceHeroId, originalTargetHeroId, debuff.weakLayers);
        }
        return;
    }

    Tower& tower = towers_[static_cast<std::size_t>(defendingPlayerId())];
    if (damageResult.towerDestroyed || tower.isDestroyed()) {
        return;
    }

    if (debuff.vulnerableLayers > 0) {
        const int previousLayers = tower.vulnerableLayers();
        tower.addVulnerableLayers(debuff.vulnerableLayers);
        CombatEvent event = baseEvent(CombatEventType::VulnerableApplied,
                                      sourceHeroId, std::nullopt,
                                      actingPlayerId_, defendingPlayerId());
        event.targetType = CombatTargetType::Tower;
        event.targetTowerPlayerId = defendingPlayerId();
        event.redirectedBecauseHeroDied = true;
        event.amount = debuff.vulnerableLayers;
        event.vulnerableLayers = tower.vulnerableLayers();
        event.previousLayers = previousLayers;
        event.addedLayers = debuff.vulnerableLayers;
        event.totalLayers = tower.vulnerableLayers();
        emitEvent(std::move(event));
    }

    if (debuff.weakLayers > 0) {
        const int previousLayers = tower.weakLayers();
        tower.addWeakLayers(debuff.weakLayers);
        CombatEvent event = baseEvent(CombatEventType::WeakApplied,
                                      sourceHeroId, std::nullopt,
                                      actingPlayerId_, defendingPlayerId());
        event.targetType = CombatTargetType::Tower;
        event.targetTowerPlayerId = defendingPlayerId();
        event.redirectedBecauseHeroDied = true;
        event.amount = debuff.weakLayers;
        event.weakLayers = tower.weakLayers();
        event.previousLayers = previousLayers;
        event.addedLayers = debuff.weakLayers;
        event.totalLayers = tower.weakLayers();
        emitEvent(std::move(event));
    }
}

void CombatContext::convertDeadHeroesToBoxes() {
    for (const DeadHeroInfo& dead : board_.convertDeadHeroesToBoxes()) {
        CombatEvent converted;
        converted.type = CombatEventType::HeroConvertedToBox;
        converted.targetHeroId = dead.id;
        converted.actingPlayerId = actingPlayerId_;
        converted.targetPlayerId = playerIdForPosition(dead.position);
        emitEvent(std::move(converted));
    }
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
    event.targetType = CombatTargetType::Hero;
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
    event.targetType = CombatTargetType::Hero;
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
