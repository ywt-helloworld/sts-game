#include "game/Hero.hpp"

#include "game/CombatContext.hpp"
#include "game/CombatMath.hpp"

#include <algorithm>
#include <stdexcept>

namespace sts {
namespace {

DamageResult performNormalAttack(Hero& attacker, CombatContext& context, int baseDamage,
                                 std::optional<HeroId>& targetId) {
    targetId = context.firstEnemyInNormalAttackOrder();
    if (targetId.has_value()) {
        return context.damageHero(attacker.id(), *targetId, baseDamage, DamageKind::NormalAttack);
    }
    return context.damageTower(attacker.id(), baseDamage, DamageKind::NormalAttack);
}

CombatEvent heroEvent(CombatEventType type, const Hero& hero, const CombatContext& context) {
    CombatEvent event;
    event.type = type;
    event.attackerHeroId = hero.id();
    event.actingPlayerId = context.actingPlayerId();
    event.targetPlayerId = context.defendingPlayerId();
    return event;
}

} // namespace

Hero::Hero(HeroId id,
           PieceColor color,
           Position position,
           int attributeValue,
           HeroStats stats,
           std::optional<int> creatorPlayerId)
    : BoardPiece(color, position),
      id_(id),
      attributeValue_(attributeValue),
      currentHp_(stats.maxHp),
      maxHp_(stats.maxHp),
      attackPower_(stats.attack),
      defense_(stats.defense),
      creatorPlayerId_(creatorPlayerId) {
    if (id == 0 || attributeValue < 1 || stats.maxHp < 1 || stats.attack < 1 || stats.defense < 0) {
        throw std::invalid_argument("hero id, attributeValue and hp/attack must be positive; defense cannot be negative");
    }
}

int Hero::receiveDamage(int rawDamage, int ignoredDefense) noexcept {
    if (!isAlive() || rawDamage <= 0) {
        return 0;
    }
    const int effectiveDefense = std::max(0, defense_ - std::max(0, ignoredDefense));
    const int afterDefense = std::max(1, rawDamage - effectiveDefense);
    const int absorbed = std::min(status_.shield, afterDefense);
    status_.shield -= absorbed;
    const int hpDamage = std::min(currentHp_, afterDefense - absorbed);
    currentHp_ -= hpDamage;
    return hpDamage;
}

int Hero::heal(int amount) noexcept {
    if (!isAlive() || amount <= 0) {
        return 0;
    }
    const int healed = std::min(amount, maxHp_ - currentHp_);
    currentHp_ += healed;
    return healed;
}

int Hero::gainShield(int amount) {
    if (!isAlive() || amount <= 0) {
        return 0;
    }
    status_.shield = checkedAdd(status_.shield, amount);
    return amount;
}

void Hero::addVulnerableLayers(int layers) {
    if (layers <= 0) {
        return;
    }
    status_.vulnerableLayers = checkedAdd(status_.vulnerableLayers, layers);
}

void Hero::addWeakLayers(int layers) {
    if (layers <= 0) {
        return;
    }
    status_.weakLayers = checkedAdd(status_.weakLayers, layers);
}

void Hero::processTurnEndStatuses() noexcept {
    status_.vulnerableLayers = std::max(0, status_.vulnerableLayers - 1);
    status_.weakLayers = std::max(0, status_.weakLayers - 1);
}

HeroStats IronFighter::makeStats(int attributeValue) {
    return {.maxHp = checkedMultiply(attributeValue, 16),
            .attack = checkedMultiply(attributeValue, 14),
            .defense = 0};
}

IronFighter::IronFighter(HeroId id, Position position, int attributeValue,
                         std::optional<int> creatorPlayerId)
    : Hero(id, PieceColor::Red, position, attributeValue, makeStats(attributeValue), creatorPlayerId) {}

void IronFighter::performAttack(CombatContext& context) {
    std::optional<HeroId> targetId;
    const DamageResult result = performNormalAttack(*this, context, currentBaseAttackDamage(), targetId);
    if (targetId.has_value()) {
        context.applyDebuffAfterHeroHit(id(), *targetId, result, {.vulnerableLayers = 3});
    }
    static_cast<void>(context.healHero(id(), checkedMultiply(attributeValue(), 2)));
}

HeroStats SilentHunter::makeStats(int attributeValue) {
    return {.maxHp = checkedMultiply(attributeValue, 14),
            .attack = checkedMultiply(attributeValue, 12),
            .defense = 0};
}

SilentHunter::SilentHunter(HeroId id, Position position, int attributeValue,
                           std::optional<int> creatorPlayerId)
    : Hero(id, PieceColor::Green, position, attributeValue, makeStats(attributeValue), creatorPlayerId) {}

void SilentHunter::performAttack(CombatContext& context) {
    std::optional<HeroId> targetId;
    const DamageResult result = performNormalAttack(*this, context, currentBaseAttackDamage(), targetId);
    if (targetId.has_value()) {
        context.applyDebuffAfterHeroHit(id(), *targetId, result, {.weakLayers = 2});
    }
    static_cast<void>(context.gainShield(id(), checkedMultiply(attributeValue(), 8)));
}

HeroStats Regent::makeStats(int attributeValue) {
    return {.maxHp = checkedMultiply(attributeValue, 15),
            .attack = checkedMultiply(attributeValue, 14),
            .defense = 0};
}

Regent::Regent(HeroId id, Position position, int attributeValue,
               std::optional<int> creatorPlayerId)
    : Hero(id, PieceColor::Yellow, position, attributeValue, makeStats(attributeValue), creatorPlayerId),
      strongAttackDamage_(checkedMultiply(attributeValue, 14)),
      chargingAttackDamage_(checkedMultiply(attributeValue, 6)) {}

int Regent::currentBaseAttackDamage() const noexcept {
    return radiantStars_ >= 2 ? strongAttackDamage_ : chargingAttackDamage_;
}

void Regent::performAttack(CombatContext& context) {
    const bool empowered = radiantStars_ >= 2;
    CombatEvent modeEvent = heroEvent(CombatEventType::RegentAttackModeSelected, *this, context);
    modeEvent.amount = empowered ? 1 : 0;
    modeEvent.radiantStars = radiantStars_;
    context.emitEvent(std::move(modeEvent));

    std::optional<HeroId> targetId;
    const DamageResult result = performNormalAttack(*this, context, currentBaseAttackDamage(), targetId);
    if (empowered) {
        if (targetId.has_value()) {
            context.applyDebuffAfterHeroHit(
                id(), *targetId, result, {.vulnerableLayers = 1, .weakLayers = 2});
        }
        radiantStars_ -= 2;
    } else {
        static_cast<void>(context.gainShield(id(), checkedMultiply(attributeValue(), 5)));
        radiantStars_ = checkedAdd(radiantStars_, 3);
    }

    CombatEvent changed = heroEvent(CombatEventType::RadiantStarsChanged, *this, context);
    changed.amount = empowered ? -2 : 3;
    changed.radiantStars = radiantStars_;
    context.emitEvent(std::move(changed));
}

HeroStats ChickenPot::makeStats(int attributeValue) {
    return {.maxHp = checkedMultiply(attributeValue, 15),
            .attack = checkedMultiply(attributeValue, 6),
            .defense = 0};
}

ChickenPot::ChickenPot(HeroId id, Position position, int attributeValue,
                       std::optional<int> creatorPlayerId)
    : Hero(id, PieceColor::Blue, position, attributeValue, makeStats(attributeValue), creatorPlayerId),
      lightningDamage_(checkedMultiply(attributeValue, 8)) {}

void ChickenPot::performAttack(CombatContext& context) {
    const bool charged = lightningOrbs_ >= 1;
    std::optional<HeroId> normalTarget;
    static_cast<void>(performNormalAttack(*this, context, currentBaseAttackDamage(), normalTarget));

    if (!charged) {
        static_cast<void>(context.gainShield(id(), checkedMultiply(attributeValue(), 5)));
        lightningOrbs_ = checkedAdd(lightningOrbs_, 1);
        CombatEvent changed = heroEvent(CombatEventType::LightningOrbChanged, *this, context);
        changed.amount = 1;
        changed.lightningOrbs = lightningOrbs_;
        context.emitEvent(std::move(changed));
        return;
    }

    if (!context.gameFinished()) {
        context.emitEvent(heroEvent(CombatEventType::LightningAttackStarted, *this, context));
        for (int activation = 0;
             activation < LightningActivationCount && !context.gameFinished();
             ++activation) {
            const LightningTarget target = context.chooseRandomLightningTarget();
            CombatEvent activatedEvent = heroEvent(CombatEventType::LightningActivated, *this, context);
            activatedEvent.amount = activation + 1;
            activatedEvent.damageKind = DamageKind::Lightning;
            context.emitEvent(std::move(activatedEvent));

            CombatEvent selected = heroEvent(CombatEventType::LightningTargetSelected, *this, context);
            selected.targetHeroId = target.heroId;
            selected.targetPlayerId = target.targetPlayerId;
            selected.amount = target.kind == LightningTargetKind::Tower ? 1 : 0;
            selected.damageKind = DamageKind::Lightning;
            context.emitEvent(std::move(selected));

            if (target.kind == LightningTargetKind::Hero) {
                static_cast<void>(
                    context.damageHero(id(), *target.heroId, lightningDamage_, DamageKind::Lightning));
            } else {
                static_cast<void>(
                    context.damageTower(id(), lightningDamage_, DamageKind::Lightning));
            }
        }
    }

    --lightningOrbs_;
    CombatEvent changed = heroEvent(CombatEventType::LightningOrbChanged, *this, context);
    changed.amount = -1;
    changed.lightningOrbs = lightningOrbs_;
    context.emitEvent(std::move(changed));
}

} // namespace sts
