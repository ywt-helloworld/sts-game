#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

#include "game/CombatMath.hpp"
#include "game/DamageResolver.hpp"
#include "game/Hero.hpp"
#include "game/Tower.hpp"

#include <limits>
#include <stdexcept>

using namespace sts;

namespace {

void factoryAndStatsTests() {
    const auto red = HeroFactory::create(1, PieceColor::Red, {5, 0}, 4, 0);
    REQUIRE(dynamic_cast<IronFighter*>(red.get()) != nullptr);
    REQUIRE(red->heroType() == HeroType::IronFighter);
    REQUIRE(red->maxHp() == 64);
    REQUIRE(red->currentBaseAttackDamage() == 56);
    REQUIRE(red->defense() == 0);

    const auto green = HeroFactory::create(2, PieceColor::Green, {5, 1}, 4, 0);
    REQUIRE(dynamic_cast<SilentHunter*>(green.get()) != nullptr);
    REQUIRE(green->heroType() == HeroType::SilentHunter);
    REQUIRE(green->maxHp() == 56);
    REQUIRE(green->currentBaseAttackDamage() == 48);
    REQUIRE(green->defense() == 0);

    const auto yellow = HeroFactory::create(3, PieceColor::Yellow, {5, 2}, 4, 0);
    REQUIRE(dynamic_cast<Regent*>(yellow.get()) != nullptr);
    REQUIRE(yellow->heroType() == HeroType::Regent);
    REQUIRE(yellow->maxHp() == 60);
    REQUIRE(yellow->currentBaseAttackDamage() == 56);
    REQUIRE(yellow->radiantStars() == Regent::InitialRadiantStars);
    REQUIRE(yellow->defense() == 0);

    const auto blue = HeroFactory::create(4, PieceColor::Blue, {5, 3}, 4, 0);
    REQUIRE(dynamic_cast<ChickenPot*>(blue.get()) != nullptr);
    REQUIRE(blue->heroType() == HeroType::ChickenPot);
    REQUIRE(blue->maxHp() == 60);
    REQUIRE(blue->currentBaseAttackDamage() == 24);
    REQUIRE(blue->lightningOrbs() == ChickenPot::InitialLightningOrbs);
    REQUIRE(blue->defense() == 0);

    BoxBoard board = test::solidBoard();
    board.setPieceAt({5, 0}, HeroFactory::create(10, PieceColor::Red, {5, 0}, 3, 0));
    board.setPieceAt({5, 1}, HeroFactory::create(11, PieceColor::Green, {5, 1}, 3, 0));
    board.setPieceAt({5, 2}, HeroFactory::create(12, PieceColor::Yellow, {5, 2}, 3, 0));
    board.setPieceAt({5, 3}, HeroFactory::create(13, PieceColor::Blue, {5, 3}, 3, 0));
    for (int column = 0; column < 4; ++column) {
        const PieceSnapshot& snapshot = board.snapshot()[5][column];
        REQUIRE(snapshot.type == PieceType::Hero);
        REQUIRE(snapshot.defense == 0);
    }
}

void integerMathTests() {
    REQUIRE(roundPositiveRatio(5, 3, 4) == 4);
    REQUIRE(roundPositiveRatio(5, 3, 2) == 8);
    REQUIRE(roundPositiveRatio(14, 3, 4) == 11);
    REQUIRE(roundPositiveRatio(11, 3, 2) == 17);
    REQUIRE(checkedMultiply(7, 14) == 98);

    bool overflowed = false;
    try {
        static_cast<void>(checkedMultiply(std::numeric_limits<int>::max(), 2));
    } catch (const std::overflow_error&) {
        overflowed = true;
    }
    REQUIRE(overflowed);
}

void damageOrderTests() {
    DamageResolver resolver;
    auto attacker = HeroFactory::create(20, PieceColor::Red, {5, 0}, 1, 0);
    auto target = HeroFactory::create(21, PieceColor::Yellow, {4, 0}, 10, 1);

    const int initialHp = target->currentHp();
    const DamageResult plain = resolver.resolveHeroDamage(
        *attacker, *target, attacker->currentBaseAttackDamage(), DamageKind::NormalAttack);
    REQUIRE(plain.baseDamage == 14);
    REQUIRE(plain.damageAfterWeak == 14);
    REQUIRE(plain.damageAfterVulnerable == 14);
    REQUIRE(plain.damageAfterDefense == 14);
    REQUIRE(plain.shieldAbsorbed == 0);
    REQUIRE(plain.hpDamage == 14);
    REQUIRE(target->currentHp() == initialHp - 14);

    attacker->addWeakLayers(2);
    target->addVulnerableLayers(2);
    REQUIRE(target->gainShield(10) == 10);
    const int hpBeforeModified = target->currentHp();
    const DamageResult modified = resolver.resolveHeroDamage(
        *attacker, *target, 14, DamageKind::NormalAttack);
    REQUIRE(modified.damageAfterWeak == 11);
    REQUIRE(modified.damageAfterVulnerable == 17);
    REQUIRE(modified.damageAfterDefense == 17);
    REQUIRE(modified.shieldAbsorbed == 10);
    REQUIRE(modified.hpDamage == 7);
    REQUIRE(modified.remainingShield == 0);
    REQUIRE(target->currentHp() == hpBeforeModified - 7);

    REQUIRE(target->gainShield(30) == 30);
    const int hpBeforeShield = target->currentHp();
    const DamageResult shielded = resolver.resolveHeroDamage(
        *attacker, *target, 14, DamageKind::NormalAttack);
    REQUIRE(shielded.shieldAbsorbed == 17);
    REQUIRE(shielded.hpDamage == 0);
    REQUIRE(target->currentHp() == hpBeforeShield);

    const DamageResult lightning = resolver.resolveHeroDamage(
        *attacker, *target, 14, DamageKind::Lightning);
    REQUIRE(lightning.damageAfterWeak == 14);
    REQUIRE(lightning.damageAfterVulnerable == 21);
    REQUIRE(lightning.damageAfterDefense == 21);

    Tower normalTower(1);
    const DamageResult weakTower = resolver.resolveTowerDamage(
        *attacker, normalTower, 14, DamageKind::NormalAttack);
    REQUIRE(weakTower.hpDamage == 11);
    Tower lightningTower(1);
    const DamageResult lightningTowerDamage = resolver.resolveTowerDamage(
        *attacker, lightningTower, 14, DamageKind::Lightning);
    REQUIRE(lightningTowerDamage.hpDamage == 14);
}

void statusLifetimeTests() {
    auto hero = HeroFactory::create(30, PieceColor::Green, {5, 0}, 2, 0);
    hero->addVulnerableLayers(2);
    hero->addWeakLayers(2);
    REQUIRE(hero->gainShield(9) == 9);
    hero->processTurnEndStatuses();
    REQUIRE(hero->vulnerableLayers() == 1);
    REQUIRE(hero->weakLayers() == 1);
    REQUIRE(hero->shield() == 9);

    DamageResolver resolver;
    auto cleanAttacker = HeroFactory::create(31, PieceColor::Red, {4, 0}, 1, 1);
    const DamageResult vulnerableDuringNextTurn = resolver.resolveHeroDamage(
        *cleanAttacker, *hero, 14, DamageKind::NormalAttack);
    REQUIRE(vulnerableDuringNextTurn.damageAfterVulnerable == 21);
    REQUIRE(vulnerableDuringNextTurn.shieldAbsorbed == 9);
    REQUIRE(hero->vulnerableLayers() == 1);

    auto cleanTarget = HeroFactory::create(32, PieceColor::Yellow, {4, 1}, 10, 1);
    const DamageResult weakDuringNextTurn = resolver.resolveHeroDamage(
        *hero, *cleanTarget, 14, DamageKind::NormalAttack);
    REQUIRE(weakDuringNextTurn.damageAfterWeak == 11);
    REQUIRE(hero->weakLayers() == 1);

    hero->processTurnEndStatuses();
    REQUIRE(hero->vulnerableLayers() == 0);
    REQUIRE(hero->weakLayers() == 0);
    REQUIRE(hero->shield() == 0);
}

void statusStackingAndFixedMultiplierTests() {
    auto statusHero = HeroFactory::create(40, PieceColor::Red, {4, 0}, 10, 1);
    statusHero->addVulnerableLayers(2);
    REQUIRE(statusHero->vulnerableLayers() == 2);
    statusHero->addVulnerableLayers(1);
    REQUIRE(statusHero->vulnerableLayers() == 3);
    statusHero->addVulnerableLayers(2);
    REQUIRE(statusHero->vulnerableLayers() == 5);
    statusHero->addVulnerableLayers(0);
    statusHero->addVulnerableLayers(-1);
    REQUIRE(statusHero->vulnerableLayers() == 5);

    statusHero->addWeakLayers(2);
    REQUIRE(statusHero->weakLayers() == 2);
    statusHero->addWeakLayers(2);
    REQUIRE(statusHero->weakLayers() == 4);
    statusHero->addWeakLayers(1);
    REQUIRE(statusHero->weakLayers() == 5);
    statusHero->addWeakLayers(0);
    statusHero->addWeakLayers(-1);
    REQUIRE(statusHero->weakLayers() == 5);

    DamageResolver resolver;
    for (const int layers : {1, 2, 3, 5}) {
        auto attacker = HeroFactory::create(50 + layers, PieceColor::Red, {5, 0}, 1, 0);
        auto target = HeroFactory::create(60 + layers, PieceColor::Yellow, {4, 0}, 10, 1);
        target->addVulnerableLayers(layers);
        const DamageResult result = resolver.resolveHeroDamage(
            *attacker, *target, 14, DamageKind::NormalAttack);
        REQUIRE(result.damageAfterVulnerable == 21);
        REQUIRE(result.hpDamage == 21);
    }

    for (const int layers : {1, 2, 5}) {
        auto attacker = HeroFactory::create(70 + layers, PieceColor::Red, {5, 0}, 1, 0);
        auto target = HeroFactory::create(80 + layers, PieceColor::Yellow, {4, 0}, 10, 1);
        attacker->addWeakLayers(layers);
        const DamageResult result = resolver.resolveHeroDamage(
            *attacker, *target, 14, DamageKind::NormalAttack);
        REQUIRE(result.damageAfterWeak == 11);
        REQUIRE(result.hpDamage == 11);
    }

    auto overflowHero = HeroFactory::create(90, PieceColor::Green, {4, 0}, 1, 1);
    overflowHero->addVulnerableLayers(std::numeric_limits<int>::max());
    bool vulnerableOverflow = false;
    try {
        overflowHero->addVulnerableLayers(1);
    } catch (const std::overflow_error&) {
        vulnerableOverflow = true;
    }
    REQUIRE(vulnerableOverflow);
}

} // namespace

void runHeroTests() {
    factoryAndStatsTests();
    integerMathTests();
    damageOrderTests();
    statusLifetimeTests();
    statusStackingAndFixedMultiplierTests();
}
