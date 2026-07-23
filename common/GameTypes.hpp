#pragma once

#include <cstdint>
#include <optional>

namespace sts {

using HeroId = std::uint64_t;

enum class HeroType { None, IronFighter, SilentHunter, Regent, ChickenPot };
enum class GamePhase { WaitingForPlayers, Elimination, Combat, Finished };
enum class DamageKind { NormalAttack, Lightning };
enum class TowerDamageSource { DirectAttack, Overflow };
enum class CombatTargetType { Hero, Tower };

struct HeroStatus {
    int vulnerableLayers{};
    int weakLayers{};
    int shield{};

    bool operator==(const HeroStatus&) const = default;
};

struct HeroStats {
    int maxHp{};
    int attack{};
    int defense{};

    bool operator==(const HeroStats&) const = default;
};

struct TowerSnapshot {
    int playerId{};
    int currentHp{};
    int maxHp{};
    int vulnerableLayers{};
    int weakLayers{};
    bool destroyed{};

    bool operator==(const TowerSnapshot&) const = default;
};

enum class CombatEventType {
    OpeningTurnCompleted,
    CombatStarted,
    HeroAttacked,
    HeroDamaged,

    VulnerableApplied,
    VulnerableReduced,
    VulnerableExpired,

    WeakApplied,
    WeakReduced,
    WeakExpired,

    ShieldGained,
    ShieldAbsorbed,
    ShieldBroken,

    HeroHealed,

    RadiantStarsChanged,
    RegentAttackModeSelected,

    LightningAttackStarted,
    LightningActivated,
    LightningTargetSelected,
    LightningOrbChanged,

    HeroDied,
    OverflowDamageGenerated,
    HeroConvertedToBox,
    TowerDamaged,
    TowerDestroyed,
    CombatFinished,
    TurnChanged,
    GameFinished
};

struct CombatEvent {
    CombatEventType type{CombatEventType::CombatStarted};
    std::optional<HeroId> attackerHeroId;
    std::optional<HeroId> targetHeroId;
    std::optional<int> actingPlayerId;
    std::optional<int> targetPlayerId;
    int damage{};
    int remainingHp{};
    std::optional<int> winnerPlayerId;
    DamageKind damageKind{DamageKind::NormalAttack};
    int baseDamage{};
    int damageAfterWeak{};
    int damageAfterVulnerable{};
    int damageAfterDefense{};
    int shieldAbsorbed{};
    int remainingShield{};
    int amount{};
    int vulnerableLayers{};
    int weakLayers{};
    int radiantStars{};
    int lightningOrbs{};
    int previousLayers{};
    int addedLayers{};
    int totalLayers{};
    int calculatedDamage{};
    int hpDamageApplied{};
    int targetRemainingHp{};
    TowerDamageSource towerDamageSource{TowerDamageSource::DirectAttack};
    int shieldBefore{};
    int shieldAfter{};
    int hpBefore{};
    int hpAfter{};
    int overflowDamage{};
    int towerHpBefore{};
    int towerDamageApplied{};
    int towerHpAfter{};
    bool towerDestroyed{};
    CombatTargetType targetType{CombatTargetType::Hero};
    std::optional<int> targetTowerPlayerId;
    bool redirectedBecauseHeroDied{};

    bool operator==(const CombatEvent&) const = default;
};

enum class LightningTargetKind { Hero, Tower };

struct LightningTarget {
    LightningTargetKind kind{LightningTargetKind::Tower};
    std::optional<HeroId> heroId;
    int targetPlayerId{};

    bool operator==(const LightningTarget&) const = default;
};

} // namespace sts
