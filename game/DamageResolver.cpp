#include "game/DamageResolver.hpp"

#include "game/CombatMath.hpp"
#include "game/Hero.hpp"
#include "game/Tower.hpp"

#include <algorithm>
#include <stdexcept>

namespace sts {

DamageResult DamageResolver::resolveHeroDamage(Hero& attacker,
                                                Hero& target,
                                                int baseDamage,
                                                DamageKind kind) const {
    if (baseDamage < 0) {
        throw std::invalid_argument("base damage cannot be negative");
    }

    DamageResult result;
    result.damageKind = kind;
    result.baseDamage = baseDamage;
    result.damageAfterWeak = kind == DamageKind::NormalAttack && attacker.isWeak()
                                 ? roundPositiveRatio(baseDamage, 3, 4)
                                 : baseDamage;
    result.damageAfterVulnerable = target.isVulnerable()
                                       ? roundPositiveRatio(result.damageAfterWeak, 3, 2)
                                       : result.damageAfterWeak;
    result.damageAfterDefense = std::max(1, result.damageAfterVulnerable - target.defense());
    result.calculatedDamage = result.damageAfterDefense;

    result.shieldBefore = target.status_.shield;
    result.hpBefore = target.currentHp_;
    result.shieldAbsorbed = std::min(result.shieldBefore, result.damageAfterDefense);
    target.status_.shield -= result.shieldAbsorbed;
    const int unshieldedDamage = result.damageAfterDefense - result.shieldAbsorbed;
    result.hpDamageApplied = std::min(result.hpBefore, unshieldedDamage);
    result.overflowDamage = std::max(0, unshieldedDamage - result.hpBefore);
    target.currentHp_ -= result.hpDamageApplied;
    result.hpDamage = result.hpDamageApplied;
    result.remainingShield = target.status_.shield;
    result.remainingHp = target.currentHp_;
    result.targetRemainingHp = result.remainingHp;
    result.shieldAfter = result.remainingShield;
    result.hpAfter = result.remainingHp;
    result.targetDied = !target.isAlive();
    return result;
}

DamageResult DamageResolver::resolveTowerDamage(Hero& attacker,
                                                 Tower& tower,
                                                 int baseDamage,
                                                 DamageKind kind) const {
    if (baseDamage < 0) {
        throw std::invalid_argument("base damage cannot be negative");
    }

    DamageResult result;
    result.damageKind = kind;
    result.baseDamage = baseDamage;
    result.damageAfterWeak = kind == DamageKind::NormalAttack && attacker.isWeak()
                                 ? roundPositiveRatio(baseDamage, 3, 4)
                                 : baseDamage;
    result.damageAfterVulnerable = tower.isVulnerable()
                                       ? roundPositiveRatio(result.damageAfterWeak, 3, 2)
                                       : result.damageAfterWeak;
    result.damageAfterDefense = result.damageAfterVulnerable;
    result.calculatedDamage = result.damageAfterDefense;
    result.towerHpBefore = tower.currentHp();
    result.hpDamageApplied = tower.takeDamage(result.damageAfterDefense);
    result.hpDamage = result.hpDamageApplied;
    result.remainingHp = tower.currentHp();
    result.targetRemainingHp = result.remainingHp;
    result.towerDamageApplied = result.hpDamageApplied;
    result.towerHpAfter = result.remainingHp;
    result.towerDestroyed = tower.isDestroyed();
    result.targetDied = tower.isDestroyed();
    return result;
}

} // namespace sts
