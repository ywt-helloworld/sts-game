#pragma once

#include "common/GameTypes.hpp"
#include "game/BoardPiece.hpp"

#include <optional>

namespace sts {

class CombatContext;
class DamageResolver;

class Hero : public BoardPiece {
public:
    Hero(HeroId id,
         PieceColor color,
         Position position,
         int attributeValue,
         HeroStats stats,
         std::optional<int> creatorPlayerId = std::nullopt);
    ~Hero() override = default;

    [[nodiscard]] PieceType type() const noexcept override { return PieceType::Hero; }
    [[nodiscard]] int inheritedAttribute() const noexcept override { return attributeValue_; }
    [[nodiscard]] HeroId id() const noexcept { return id_; }
    [[nodiscard]] int attributeValue() const noexcept { return attributeValue_; }
    [[nodiscard]] int currentHp() const noexcept { return currentHp_; }
    [[nodiscard]] int maxHp() const noexcept { return maxHp_; }
    [[nodiscard]] int attackPower() const noexcept { return currentBaseAttackDamage(); }
    [[nodiscard]] int attackValue() const noexcept { return currentBaseAttackDamage(); }
    [[nodiscard]] int defense() const noexcept { return defense_; }
    [[nodiscard]] int defenseValue() const noexcept { return defense_; }
    [[nodiscard]] int shield() const noexcept { return status_.shield; }
    [[nodiscard]] int vulnerableLayers() const noexcept { return status_.vulnerableLayers; }
    [[nodiscard]] int weakLayers() const noexcept { return status_.weakLayers; }
    [[nodiscard]] bool isAlive() const noexcept { return currentHp_ > 0; }
    [[nodiscard]] bool isDead() const noexcept { return !isAlive(); }
    [[nodiscard]] bool isVulnerable() const noexcept { return status_.vulnerableLayers > 0; }
    [[nodiscard]] bool isWeak() const noexcept { return status_.weakLayers > 0; }
    [[nodiscard]] std::optional<int> creatorPlayerId() const noexcept { return creatorPlayerId_; }

    [[nodiscard]] int receiveDamage(int rawDamage, int ignoredDefense = 0) noexcept;
    [[nodiscard]] int heal(int amount) noexcept;
    [[nodiscard]] int gainShield(int amount);
    void addVulnerableLayers(int layers);
    void addWeakLayers(int layers);
    void processTurnEndStatuses() noexcept;

    [[nodiscard]] virtual HeroType heroType() const noexcept = 0;
    [[nodiscard]] virtual int currentBaseAttackDamage() const noexcept = 0;
    [[nodiscard]] virtual int radiantStars() const noexcept { return 0; }
    [[nodiscard]] virtual int lightningOrbs() const noexcept { return 0; }
    virtual void performAttack(CombatContext& context) = 0;

    void onSpawn() override {}
    void onEliminated() override {}

protected:
    HeroId id_{};
    int attributeValue_{};
    int currentHp_{};
    int maxHp_{};
    int attackPower_{};
    int defense_{};
    HeroStatus status_{};
    std::optional<int> creatorPlayerId_;

private:
    friend class DamageResolver;
};

class IronFighter final : public Hero {
public:
    [[nodiscard]] static HeroStats makeStats(int attributeValue);
    IronFighter(HeroId id, Position position, int attributeValue,
                std::optional<int> creatorPlayerId = std::nullopt);
    [[nodiscard]] HeroType heroType() const noexcept override { return HeroType::IronFighter; }
    [[nodiscard]] int currentBaseAttackDamage() const noexcept override { return attackPower_; }
    void performAttack(CombatContext& context) override;
};

class SilentHunter final : public Hero {
public:
    [[nodiscard]] static HeroStats makeStats(int attributeValue);
    SilentHunter(HeroId id, Position position, int attributeValue,
                 std::optional<int> creatorPlayerId = std::nullopt);
    [[nodiscard]] HeroType heroType() const noexcept override { return HeroType::SilentHunter; }
    [[nodiscard]] int currentBaseAttackDamage() const noexcept override { return attackPower_; }
    void performAttack(CombatContext& context) override;
};

class Regent final : public Hero {
public:
    static constexpr int InitialRadiantStars = 3;

    [[nodiscard]] static HeroStats makeStats(int attributeValue);
    Regent(HeroId id, Position position, int attributeValue,
           std::optional<int> creatorPlayerId = std::nullopt);
    [[nodiscard]] HeroType heroType() const noexcept override { return HeroType::Regent; }
    [[nodiscard]] int currentBaseAttackDamage() const noexcept override;
    [[nodiscard]] int radiantStars() const noexcept override { return radiantStars_; }
    void performAttack(CombatContext& context) override;

private:
    int strongAttackDamage_{};
    int chargingAttackDamage_{};
    int radiantStars_{InitialRadiantStars};
};

class ChickenPot final : public Hero {
public:
    static constexpr int InitialLightningOrbs = 1;
    static constexpr int LightningActivationCount = 2;

    [[nodiscard]] static HeroStats makeStats(int attributeValue);
    ChickenPot(HeroId id, Position position, int attributeValue,
               std::optional<int> creatorPlayerId = std::nullopt);
    [[nodiscard]] HeroType heroType() const noexcept override { return HeroType::ChickenPot; }
    [[nodiscard]] int currentBaseAttackDamage() const noexcept override { return attackPower_; }
    [[nodiscard]] int lightningOrbs() const noexcept override { return lightningOrbs_; }
    void performAttack(CombatContext& context) override;

private:
    int lightningDamage_{};
    int lightningOrbs_{InitialLightningOrbs};
};

// Source-compatible names for older code; the runtime HeroType and behavior use
// the four formal class names above.
using RedHero = IronFighter;
using GreenHero = SilentHunter;
using YellowHero = Regent;
using BlueHero = ChickenPot;

} // namespace sts
