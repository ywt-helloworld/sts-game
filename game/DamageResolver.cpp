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
    result.baseDamage = baseDamage;
    result.damageAfterWeak = kind == DamageKind::NormalAttack && attacker.isWeak()
                                 ? roundPositiveRatio(baseDamage, 3, 4)
                                 : baseDamage;
    result.damageAfterVulnerable = target.isVulnerable()
                                       ? roundPositiveRatio(result.damageAfterWeak, 3, 2)
                                       : result.damageAfterWeak;
    result.damageAfterDefense = std::max(1, result.damageAfterVulnerable - target.defense());
    result.calculatedDamage = result.damageAfterDefense;

    result.shieldAbsorbed = std::min(target.status_.shield, result.damageAfterDefense);
    target.status_.shield -= result.shieldAbsorbed;
    const int unshieldedDamage = result.damageAfterDefense - result.shieldAbsorbed;
    const int targetHpBeforeDamage = target.currentHp_;
    result.hpDamageApplied = std::min(targetHpBeforeDamage, unshieldedDamage);
    result.overkillDamage = std::max(0, unshieldedDamage - targetHpBeforeDamage);
    target.currentHp_ -= result.hpDamageApplied;
    result.hpDamage = result.hpDamageApplied;
    result.remainingShield = target.status_.shield;
    result.remainingHp = target.currentHp_;
    result.targetRemainingHp = result.remainingHp;
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
    result.baseDamage = baseDamage;
    result.damageAfterWeak = kind == DamageKind::NormalAttack && attacker.isWeak()
                                 ? roundPositiveRatio(baseDamage, 3, 4)
                                 : baseDamage;
    result.damageAfterVulnerable = result.damageAfterWeak;
    result.damageAfterDefense = result.damageAfterVulnerable;
    result.calculatedDamage = result.damageAfterDefense;
    const int towerHpBeforeDamage = tower.currentHp();
    result.hpDamageApplied = tower.takeDamage(result.damageAfterDefense);
    result.overkillDamage = std::max(0, result.damageAfterDefense - towerHpBeforeDamage);
    result.hpDamage = result.hpDamageApplied;
    result.remainingHp = tower.currentHp();
    result.targetRemainingHp = result.remainingHp;
    result.targetDied = tower.isDestroyed();
    return result;
}

} // namespace sts
