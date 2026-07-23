#pragma once

#include "common/GameTypes.hpp"

namespace sts {

class Hero;
class Tower;

struct DamageResult {
    int baseDamage{};
    int damageAfterWeak{};
    int damageAfterVulnerable{};
    int damageAfterDefense{};
    int shieldAbsorbed{};
    int hpDamage{};
    int remainingShield{};
    int remainingHp{};
    bool targetDied{};
    int calculatedDamage{};
    int hpDamageApplied{};
    int overkillDamage{};
    int targetRemainingHp{};
};

class DamageResolver {
public:
    [[nodiscard]] DamageResult resolveHeroDamage(Hero& attacker,
                                                 Hero& target,
                                                 int baseDamage,
                                                 DamageKind kind) const;
    [[nodiscard]] DamageResult resolveTowerDamage(Hero& attacker,
                                                  Tower& tower,
                                                  int baseDamage,
                                                  DamageKind kind) const;
};

} // namespace sts
