#pragma once

#include "common/GameTypes.hpp"
#include "game/DamageResolver.hpp"

#include <array>
#include <optional>
#include <vector>

namespace sts {

class BoxBoard;
class CombatRandom;
class Hero;
class Tower;

class CombatContext {
public:
    CombatContext(int actingPlayerId,
                  BoxBoard& board,
                  std::array<Tower, 2>& towers,
                  CombatRandom& random,
                  std::vector<CombatEvent>& events);

    [[nodiscard]] int actingPlayerId() const noexcept { return actingPlayerId_; }
    [[nodiscard]] int defendingPlayerId() const noexcept { return 1 - actingPlayerId_; }
    [[nodiscard]] std::vector<HeroId> livingEnemyHeroIds() const;
    [[nodiscard]] bool hasLivingEnemies() const;
    [[nodiscard]] std::optional<HeroId> firstEnemyInNormalAttackOrder() const;
    [[nodiscard]] Hero* findHero(HeroId id) noexcept;
    [[nodiscard]] const Hero* findHero(HeroId id) const noexcept;
    [[nodiscard]] DamageResult damageHero(HeroId attackerId,
                                          HeroId targetId,
                                          int baseDamage,
                                          DamageKind kind);
    [[nodiscard]] DamageResult damageTower(HeroId attackerId,
                                           int baseDamage,
                                           DamageKind kind);
    [[nodiscard]] LightningTarget chooseRandomLightningTarget();
    void addVulnerable(HeroId attackerId, HeroId targetId, int layers);
    void addWeak(HeroId attackerId, HeroId targetId, int layers);
    [[nodiscard]] int healHero(HeroId heroId, int amount);
    [[nodiscard]] int gainShield(HeroId heroId, int amount);
    void emitEvent(CombatEvent event);
    [[nodiscard]] bool gameFinished() const noexcept;

private:
    [[nodiscard]] bool isValidLivingAttacker(HeroId attackerId) const noexcept;
    [[nodiscard]] bool isValidLivingEnemy(HeroId targetId) const noexcept;

    int actingPlayerId_{};
    BoxBoard& board_;
    std::array<Tower, 2>& towers_;
    CombatRandom& random_;
    std::vector<CombatEvent>& events_;
    DamageResolver damageResolver_;
};

} // namespace sts
