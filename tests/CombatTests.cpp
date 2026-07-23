#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

#include "common/PlayerArea.hpp"
#include "common/PlayerViewTransform.hpp"
#include "game/CombatContext.hpp"
#include "game/CombatRandom.hpp"
#include "game/CombatResolver.hpp"
#include "game/Hero.hpp"
#include "game/Tower.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <ranges>
#include <vector>

namespace {

using namespace sts;

Hero* placeHero(BoxBoard& board,
                HeroId id,
                PieceColor color,
                Position position,
                int attributeValue,
                std::optional<int> creator = std::nullopt) {
    board.setPieceAt(position, HeroFactory::create(id, color, position, attributeValue, creator));
    return board.heroById(id);
}

void lowerToHp(Hero& hero, int desiredHp) {
    REQUIRE(desiredHp >= 0);
    REQUIRE(desiredHp <= hero.currentHp());
    static_cast<void>(hero.receiveDamage(hero.currentHp() - desiredHp));
    REQUIRE(hero.currentHp() == desiredHp);
}

std::size_t countEvents(const std::vector<CombatEvent>& events, CombatEventType type) {
    return static_cast<std::size_t>(std::ranges::count_if(
        events, [type](const CombatEvent& event) { return event.type == type; }));
}

const CombatEvent* firstEvent(const std::vector<CombatEvent>& events, CombatEventType type) {
    const auto found = std::ranges::find_if(
        events, [type](const CombatEvent& event) { return event.type == type; });
    return found == events.end() ? nullptr : &*found;
}

void performOne(Hero& hero,
                int actingPlayerId,
                BoxBoard& board,
                std::array<Tower, 2>& towers,
                CombatRandom& random,
                std::vector<CombatEvent>& events) {
    CombatContext context(actingPlayerId, board, towers, random, events);
    hero.performAttack(context);
}

void positionAttributeAndResetTests() {
    REQUIRE(playerIdForPosition({0, 0}) == 1);
    REQUIRE(playerIdForPosition({4, 4}) == 1);
    REQUIRE(playerIdForPosition({5, 0}) == 0);
    REQUIRE(playerIdForPosition({9, 4}) == 0);

    BoxBoard board = test::solidBoard(PieceColor::Yellow);
    Hero* old = placeHero(board, 100, PieceColor::Yellow, {5, 1}, 5, 0);
    std::array<Tower, 2> oldTowers{Tower{0}, Tower{1}};
    CombatRandom oldRandom(3U);
    std::vector<CombatEvent> oldEvents;
    performOne(*old, 0, board, oldTowers, oldRandom, oldEvents);
    REQUIRE(old->radiantStars() == 1);
    lowerToHp(*old, 20);
    REQUIRE(old->gainShield(17) == 17);
    old->addWeakLayers(3);
    old->addVulnerableLayers(2);

    const EliminationValueResult value = board.calculateEliminationValue({{5, 0}, {5, 1}, {6, 1}});
    REQUIRE(value.boxCount == 2);
    REQUIRE(value.inheritedHeroValue == 5);
    REQUIRE(value.totalAttributeValue == 7);

    const HeroId newId = board.resolveElimination(0, {{5, 0}, {5, 1}, {6, 1}});
    Hero* created = board.heroById(newId);
    REQUIRE(created != nullptr);
    REQUIRE(created->attributeValue() == 7);
    REQUIRE(created->heroType() == HeroType::Regent);
    REQUIRE(created->currentHp() == created->maxHp());
    REQUIRE(created->shield() == 0);
    REQUIRE(created->weakLayers() == 0);
    REQUIRE(created->vulnerableLayers() == 0);
    REQUIRE(created->radiantStars() == Regent::InitialRadiantStars);
}

void attackOrderTests() {
    BoxBoard board = test::solidBoard();
    placeHero(board, 1, PieceColor::Red, {6, 0}, 1, 0);
    placeHero(board, 2, PieceColor::Red, {5, 4}, 1, 0);
    placeHero(board, 3, PieceColor::Red, {5, 0}, 1, 0);
    std::array<Tower, 2> towers{Tower{0}, Tower{1}};
    REQUIRE(CombatResolver(board, towers).collectAttackers(0) == std::vector<HeroId>({3, 2, 1}));

    BoxBoard playerOneBoard = test::solidBoard();
    placeHero(playerOneBoard, 10, PieceColor::Blue, {3, 4}, 1, 1);
    placeHero(playerOneBoard, 11, PieceColor::Blue, {4, 0}, 1, 1);
    placeHero(playerOneBoard, 12, PieceColor::Blue, {4, 4}, 1, 1);
    std::array<Tower, 2> playerOneTowers{Tower{0}, Tower{1}};
    REQUIRE(CombatResolver(playerOneBoard, playerOneTowers).collectAttackers(1) ==
            std::vector<HeroId>({11, 12, 10}));

    CombatRandom random(std::vector<std::size_t>{0});
    std::vector<CombatEvent> events;
    CombatContext context0(0, board, towers, random, events);
    placeHero(board, 20, PieceColor::Green, {4, 4}, 3, 1);
    placeHero(board, 21, PieceColor::Green, {4, 0}, 3, 1);
    placeHero(board, 22, PieceColor::Green, {3, 0}, 3, 1);
    REQUIRE(context0.firstEnemyInNormalAttackOrder() == HeroId{20});
    static_cast<void>(board.heroById(20)->receiveDamage(100000));
    REQUIRE(context0.firstEnemyInNormalAttackOrder() == HeroId{21});
    static_cast<void>(board.heroById(21)->receiveDamage(100000));
    REQUIRE(context0.firstEnemyInNormalAttackOrder() == HeroId{22});

    CombatContext context1(1, board, towers, random, events);
    REQUIRE(context1.firstEnemyInNormalAttackOrder() == HeroId{3});
    const Position playerOneDisplay = PlayerViewTransform::logicalToDisplay(1, board.heroById(3)->position());
    REQUIRE(playerOneDisplay.column == BoardColumns - 1 - board.heroById(3)->position().column);
    REQUIRE(context1.firstEnemyInNormalAttackOrder() == HeroId{3});
    static_cast<void>(board.heroById(3)->receiveDamage(100000));
    REQUIRE(context1.firstEnemyInNormalAttackOrder() == HeroId{2});
    static_cast<void>(board.heroById(2)->receiveDamage(100000));
    REQUIRE(context1.firstEnemyInNormalAttackOrder() == HeroId{1});
}

void ironFighterTests() {
    BoxBoard board = test::solidBoard();
    Hero* fighter = placeHero(board, 30, PieceColor::Red, {5, 0}, 2, 0);
    Hero* target = placeHero(board, 31, PieceColor::Yellow, {4, 0}, 10, 1);
    lowerToHp(*fighter, fighter->maxHp() - 6);
    const int targetHp = target->currentHp();
    std::array<Tower, 2> towers{Tower{0}, Tower{1}};
    CombatRandom random(std::vector<std::size_t>{0});
    std::vector<CombatEvent> events;

    performOne(*fighter, 0, board, towers, random, events);
    REQUIRE(target->currentHp() == targetHp - 28);
    REQUIRE(target->vulnerableLayers() == 3);
    REQUIRE(fighter->currentHp() == fighter->maxHp() - 2);
    REQUIRE(fighter->shield() == 0);
    REQUIRE(countEvents(events, CombatEventType::VulnerableApplied) == 1U);
    REQUIRE(countEvents(events, CombatEventType::HeroHealed) == 1U);
    const CombatEvent* firstApplied = firstEvent(events, CombatEventType::VulnerableApplied);
    REQUIRE(firstApplied != nullptr);
    REQUIRE(firstApplied->previousLayers == 0);
    REQUIRE(firstApplied->addedLayers == 3);
    REQUIRE(firstApplied->totalLayers == 3);
    REQUIRE(towers[1].currentHp() == Tower::MaxHp);
    performOne(*fighter, 0, board, towers, random, events);
    performOne(*fighter, 0, board, towers, random, events);
    REQUIRE(fighter->currentHp() == fighter->maxHp());

    BoxBoard stackedBoard = test::solidBoard();
    Hero* stackedFighter = placeHero(stackedBoard, 32, PieceColor::Red, {5, 0}, 2, 0);
    Hero* vulnerableTarget = placeHero(stackedBoard, 33, PieceColor::Yellow, {4, 0}, 10, 1);
    vulnerableTarget->addVulnerableLayers(1);
    const int vulnerableHp = vulnerableTarget->currentHp();
    std::array<Tower, 2> stackedTowers{Tower{0}, Tower{1}};
    std::vector<CombatEvent> stackedEvents;
    performOne(*stackedFighter, 0, stackedBoard, stackedTowers, random, stackedEvents);
    REQUIRE(vulnerableTarget->currentHp() == vulnerableHp - 42);
    REQUIRE(vulnerableTarget->vulnerableLayers() == 4);
    const CombatEvent* stackedApplied = firstEvent(stackedEvents, CombatEventType::VulnerableApplied);
    REQUIRE(stackedApplied != nullptr);
    REQUIRE(stackedApplied->previousLayers == 1);
    REQUIRE(stackedApplied->addedLayers == 3);
    REQUIRE(stackedApplied->totalLayers == 4);

    BoxBoard fiveLayerBoard = test::solidBoard();
    Hero* fiveLayerFighter = placeHero(fiveLayerBoard, 34, PieceColor::Red, {5, 0}, 2, 0);
    Hero* twoLayerTarget = placeHero(fiveLayerBoard, 35, PieceColor::Yellow, {4, 0}, 10, 1);
    twoLayerTarget->addVulnerableLayers(2);
    std::array<Tower, 2> fiveLayerTowers{Tower{0}, Tower{1}};
    std::vector<CombatEvent> fiveLayerEvents;
    performOne(*fiveLayerFighter, 0, fiveLayerBoard, fiveLayerTowers, random, fiveLayerEvents);
    REQUIRE(twoLayerTarget->vulnerableLayers() == 5);
    const CombatEvent* fiveLayerApplied = firstEvent(fiveLayerEvents, CombatEventType::VulnerableApplied);
    REQUIRE(fiveLayerApplied != nullptr);
    REQUIRE(fiveLayerApplied->previousLayers == 2);
    REQUIRE(fiveLayerApplied->addedLayers == 3);
    REQUIRE(fiveLayerApplied->totalLayers == 5);

    BoxBoard durationBoard = test::solidBoard();
    Hero* durationFighter = placeHero(durationBoard, 36, PieceColor::Red, {5, 0}, 1, 0);
    Hero* durationTarget = placeHero(durationBoard, 37, PieceColor::Yellow, {4, 0}, 10, 1);
    std::array<Tower, 2> durationTowers{Tower{0}, Tower{1}};
    std::vector<CombatEvent> durationEvents;
    performOne(*durationFighter, 0, durationBoard, durationTowers, random, durationEvents);
    REQUIRE(durationTarget->vulnerableLayers() == 3);
    durationTarget->processTurnEndStatuses();
    REQUIRE(durationTarget->vulnerableLayers() == 2);
    durationTarget->processTurnEndStatuses();
    REQUIRE(durationTarget->vulnerableLayers() == 1);
    durationTarget->processTurnEndStatuses();
    REQUIRE(durationTarget->vulnerableLayers() == 0);
}

void silentHunterTests() {
    BoxBoard board = test::solidBoard();
    Hero* hunter = placeHero(board, 40, PieceColor::Green, {5, 0}, 2, 0);
    Hero* target = placeHero(board, 41, PieceColor::Yellow, {4, 0}, 10, 1);
    const int targetHp = target->currentHp();
    std::array<Tower, 2> towers{Tower{0}, Tower{1}};
    CombatRandom random(std::vector<std::size_t>{0});
    std::vector<CombatEvent> events;

    performOne(*hunter, 0, board, towers, random, events);
    REQUIRE(target->currentHp() == targetHp - 24);
    REQUIRE(target->weakLayers() == 2);
    REQUIRE(hunter->shield() == 16);
    performOne(*hunter, 0, board, towers, random, events);
    REQUIRE(hunter->shield() == 32);
    REQUIRE(target->weakLayers() == 4);
    REQUIRE(hunter->currentHp() == hunter->maxHp());
    REQUIRE(countEvents(events, CombatEventType::ShieldGained) == 2U);
    std::vector<const CombatEvent*> weakApplied;
    for (const CombatEvent& event : events) {
        if (event.type == CombatEventType::WeakApplied) {
            weakApplied.push_back(&event);
        }
    }
    REQUIRE(weakApplied.size() == 2U);
    REQUIRE(weakApplied[0]->previousLayers == 0);
    REQUIRE(weakApplied[0]->addedLayers == 2);
    REQUIRE(weakApplied[0]->totalLayers == 2);
    REQUIRE(weakApplied[1]->previousLayers == 2);
    REQUIRE(weakApplied[1]->addedLayers == 2);
    REQUIRE(weakApplied[1]->totalLayers == 4);
}

void regentTests() {
    BoxBoard board = test::solidBoard();
    auto* regent = dynamic_cast<Regent*>(placeHero(board, 50, PieceColor::Yellow, {5, 0}, 1, 0));
    REQUIRE(regent != nullptr);
    std::array<Tower, 2> towers{Tower{0}, Tower{1}};
    CombatRandom random(std::vector<std::size_t>{0});
    std::vector<CombatEvent> events;

    const std::array expectedStars{1, 4, 2, 0, 3};
    const std::array expectedDamage{14, 6, 14, 14, 6};
    const std::array expectedShield{0, 5, 5, 5, 10};
    for (std::size_t index = 0; index < expectedStars.size(); ++index) {
        const int towerHp = towers[1].currentHp();
        performOne(*regent, 0, board, towers, random, events);
        REQUIRE(regent->radiantStars() == expectedStars[index]);
        REQUIRE(towers[1].currentHp() == towerHp - expectedDamage[index]);
        REQUIRE(regent->shield() == expectedShield[index]);
    }

    BoxBoard targetBoard = test::solidBoard();
    Hero* strongRegent = placeHero(targetBoard, 51, PieceColor::Yellow, {5, 0}, 2, 0);
    Hero* target = placeHero(targetBoard, 52, PieceColor::Red, {4, 0}, 10, 1);
    target->addVulnerableLayers(2);
    std::array<Tower, 2> targetTowers{Tower{0}, Tower{1}};
    std::vector<CombatEvent> targetEvents;
    performOne(*strongRegent, 0, targetBoard, targetTowers, random, targetEvents);
    REQUIRE(target->weakLayers() == 2);
    REQUIRE(target->vulnerableLayers() == 3);
    REQUIRE(strongRegent->radiantStars() == 1);
    const CombatEvent* vulnerableApplied = firstEvent(targetEvents, CombatEventType::VulnerableApplied);
    REQUIRE(vulnerableApplied != nullptr);
    REQUIRE(vulnerableApplied->previousLayers == 2);
    REQUIRE(vulnerableApplied->addedLayers == 1);
    REQUIRE(vulnerableApplied->totalLayers == 3);
}

void chickenPotTests() {
    {
        BoxBoard board = test::solidBoard();
        Hero* chicken = placeHero(board, 60, PieceColor::Blue, {5, 0}, 1, 0);
        Hero* target = placeHero(board, 61, PieceColor::Red, {4, 0}, 10, 1);
        chicken->addWeakLayers(2);
        const int targetHp = target->currentHp();
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(std::vector<std::size_t>{0, 0});
        std::vector<CombatEvent> events;
        performOne(*chicken, 0, board, towers, random, events);
        REQUIRE(target->currentHp() == targetHp - 5 - 8 - 8);
        REQUIRE(chicken->lightningOrbs() == 0);
        REQUIRE(chicken->shield() == 0);
        REQUIRE(countEvents(events, CombatEventType::LightningActivated) == 2U);
        REQUIRE(countEvents(events, CombatEventType::LightningOrbChanged) == 1U);

        performOne(*chicken, 0, board, towers, random, events);
        REQUIRE(chicken->lightningOrbs() == 1);
        REQUIRE(chicken->shield() == 5);
    }
    {
        BoxBoard board = test::solidBoard();
        Hero* chicken = placeHero(board, 70, PieceColor::Blue, {5, 0}, 1, 0);
        placeHero(board, 71, PieceColor::Red, {4, 0}, 10, 1);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(std::vector<std::size_t>{1, 1});
        std::vector<CombatEvent> events;
        performOne(*chicken, 0, board, towers, random, events);
        REQUIRE(towers[1].currentHp() == Tower::MaxHp - 16);
        REQUIRE(firstEvent(events, CombatEventType::TowerDamaged)->damageKind == DamageKind::Lightning);
    }
    {
        BoxBoard board = test::solidBoard();
        Hero* chicken = placeHero(board, 80, PieceColor::Blue, {5, 0}, 1, 0);
        Hero* first = placeHero(board, 81, PieceColor::Red, {4, 4}, 1, 1);
        Hero* second = placeHero(board, 82, PieceColor::Red, {4, 3}, 10, 1);
        lowerToHp(*first, 12);
        const int secondHp = second->currentHp();
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(std::vector<std::size_t>{1, 0});
        std::vector<CombatEvent> events;
        performOne(*chicken, 0, board, towers, random, events);
        REQUIRE(first->isDead());
        REQUIRE(second->currentHp() == secondHp - 8);
        REQUIRE(chicken->lightningOrbs() == 0);
        std::vector<const CombatEvent*> lightningDamageEvents;
        for (const CombatEvent& event : events) {
            if (event.type == CombatEventType::HeroDamaged && event.damageKind == DamageKind::Lightning) {
                lightningDamageEvents.push_back(&event);
            }
        }
        REQUIRE(lightningDamageEvents.size() == 2U);
        REQUIRE(lightningDamageEvents[0]->targetHeroId == HeroId{81});
        REQUIRE(lightningDamageEvents[0]->overkillDamage == 2);
        REQUIRE(lightningDamageEvents[1]->targetHeroId == HeroId{82});
        REQUIRE(lightningDamageEvents[1]->baseDamage == 8);
    }
}

void singleTargetAndOverkillTests() {
    DamageResolver resolver;
    auto rawAttacker = HeroFactory::create(200, PieceColor::Red, {5, 0}, 10, 0);
    auto rawTarget = HeroFactory::create(201, PieceColor::Yellow, {4, 0}, 10, 1);
    lowerToHp(*rawTarget, 30);
    const DamageResult overkill = resolver.resolveHeroDamage(
        *rawAttacker, *rawTarget, 100, DamageKind::NormalAttack);
    REQUIRE(overkill.calculatedDamage == 100);
    REQUIRE(overkill.hpDamageApplied == 30);
    REQUIRE(overkill.hpDamage == 30);
    REQUIRE(overkill.overkillDamage == 70);
    REQUIRE(overkill.targetRemainingHp == 0);
    REQUIRE(overkill.targetDied);

    auto shieldedTarget = HeroFactory::create(202, PieceColor::Yellow, {4, 0}, 10, 1);
    lowerToHp(*shieldedTarget, 30);
    REQUIRE(shieldedTarget->gainShield(20) == 20);
    const DamageResult shieldedOverkill = resolver.resolveHeroDamage(
        *rawAttacker, *shieldedTarget, 100, DamageKind::NormalAttack);
    REQUIRE(shieldedOverkill.shieldAbsorbed == 20);
    REQUIRE(shieldedOverkill.hpDamageApplied == 30);
    REQUIRE(shieldedOverkill.overkillDamage == 50);
    REQUIRE(shieldedOverkill.targetRemainingHp == 0);

    const auto verifySingleNormalTarget = [](PieceColor attackerColor, HeroId firstId) {
        BoxBoard board = test::solidBoard();
        Hero* attacker = placeHero(board, firstId, attackerColor, {5, 0}, 10, 0);
        Hero* screenLeft = placeHero(board, firstId + 1, PieceColor::Red, {4, 0}, 20, 1);
        Hero* screenRight = placeHero(board, firstId + 2, PieceColor::Red, {4, 1}, 20, 1);
        const int screenLeftHp = screenLeft->currentHp();
        const int screenRightHp = screenRight->currentHp();
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(23U);
        std::vector<CombatEvent> events;
        performOne(*attacker, 0, board, towers, random, events);
        REQUIRE(screenLeft->currentHp() == screenLeftHp);
        REQUIRE(screenRight->currentHp() < screenRightHp);
        REQUIRE(towers[1].currentHp() == Tower::MaxHp);
        std::size_t normalDamageEvents = 0U;
        for (const CombatEvent& event : events) {
            if (event.type == CombatEventType::HeroDamaged &&
                event.damageKind == DamageKind::NormalAttack) {
                ++normalDamageEvents;
                REQUIRE(event.targetHeroId == screenRight->id());
            }
        }
        REQUIRE(normalDamageEvents == 1U);
    };

    verifySingleNormalTarget(PieceColor::Red, 210);
    verifySingleNormalTarget(PieceColor::Green, 220);
    verifySingleNormalTarget(PieceColor::Yellow, 230);

    {
        BoxBoard board = test::solidBoard();
        Hero* regent = placeHero(board, 240, PieceColor::Yellow, {5, 0}, 10, 0);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(29U);
        std::vector<CombatEvent> events;
        performOne(*regent, 0, board, towers, random, events);
        REQUIRE(regent->radiantStars() == 1);
        Hero* left = placeHero(board, 241, PieceColor::Red, {4, 0}, 20, 1);
        Hero* right = placeHero(board, 242, PieceColor::Red, {4, 1}, 20, 1);
        const int leftHp = left->currentHp();
        events.clear();
        performOne(*regent, 0, board, towers, random, events);
        REQUIRE(left->currentHp() == leftHp);
        REQUIRE(right->currentHp() == right->maxHp() - 60);
        REQUIRE(countEvents(events, CombatEventType::HeroDamaged) == 1U);
    }

    {
        BoxBoard board = test::solidBoard();
        Hero* chicken = placeHero(board, 250, PieceColor::Blue, {5, 0}, 10, 0);
        Hero* left = placeHero(board, 251, PieceColor::Red, {4, 0}, 20, 1);
        Hero* right = placeHero(board, 252, PieceColor::Red, {4, 1}, 20, 1);
        const int leftHp = left->currentHp();
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(std::vector<std::size_t>{2, 2});
        std::vector<CombatEvent> events;
        performOne(*chicken, 0, board, towers, random, events);
        REQUIRE(left->currentHp() == leftHp);
        REQUIRE(right->currentHp() == right->maxHp() - 60);
        std::size_t normalDamageEvents = 0U;
        for (const CombatEvent& event : events) {
            if (event.type == CombatEventType::HeroDamaged &&
                event.damageKind == DamageKind::NormalAttack) {
                ++normalDamageEvents;
                REQUIRE(event.targetHeroId == right->id());
            }
        }
        REQUIRE(normalDamageEvents == 1U);
    }

    {
        BoxBoard board = test::solidBoard();
        Hero* chicken = placeHero(board, 255, PieceColor::Blue, {5, 0}, 1, 0);
        Hero* first = placeHero(board, 256, PieceColor::Red, {4, 4}, 1, 1);
        Hero* second = placeHero(board, 257, PieceColor::Red, {4, 3}, 10, 1);
        lowerToHp(*first, 3);
        const int secondHp = second->currentHp();
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(std::vector<std::size_t>{0, 0});
        std::vector<CombatEvent> events;
        performOne(*chicken, 0, board, towers, random, events);

        REQUIRE(first->isDead());
        REQUIRE(second->currentHp() == secondHp - 16);
        std::vector<const CombatEvent*> damageEvents;
        for (const CombatEvent& event : events) {
            if (event.type == CombatEventType::HeroDamaged) {
                damageEvents.push_back(&event);
            }
        }
        REQUIRE(damageEvents.size() == 3U);
        REQUIRE(damageEvents[0]->damageKind == DamageKind::NormalAttack);
        REQUIRE(damageEvents[0]->targetHeroId == first->id());
        REQUIRE(damageEvents[0]->baseDamage == 6);
        REQUIRE(damageEvents[0]->hpDamageApplied == 3);
        REQUIRE(damageEvents[0]->overkillDamage == 3);
        REQUIRE(damageEvents[1]->damageKind == DamageKind::Lightning);
        REQUIRE(damageEvents[1]->targetHeroId == second->id());
        REQUIRE(damageEvents[1]->baseDamage == 8);
        REQUIRE(damageEvents[2]->damageKind == DamageKind::Lightning);
        REQUIRE(damageEvents[2]->targetHeroId == second->id());
        REQUIRE(damageEvents[2]->baseDamage == 8);
    }

    {
        BoxBoard board = test::solidBoard();
        Hero* attacker = placeHero(board, 258, PieceColor::Red, {5, 0}, 10, 0);
        Hero* victim = placeHero(board, 259, PieceColor::Yellow, {4, 4}, 10, 1);
        Hero* sameRow = placeHero(board, 263, PieceColor::Yellow, {4, 3}, 10, 1);
        Hero* rearRow = placeHero(board, 264, PieceColor::Yellow, {3, 4}, 10, 1);
        lowerToHp(*victim, 30);
        const int sameRowHp = sameRow->currentHp();
        const int rearRowHp = rearRow->currentHp();
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(37U);
        std::vector<CombatEvent> events;
        performOne(*attacker, 0, board, towers, random, events);

        REQUIRE(victim->isDead());
        REQUIRE(sameRow->currentHp() == sameRowHp);
        REQUIRE(rearRow->currentHp() == rearRowHp);
        REQUIRE(towers[1].currentHp() == Tower::MaxHp);
        const CombatEvent* damaged = firstEvent(events, CombatEventType::HeroDamaged);
        REQUIRE(damaged != nullptr);
        REQUIRE(damaged->targetHeroId == victim->id());
        REQUIRE(damaged->hpDamageApplied == 30);
        REQUIRE(damaged->overkillDamage == 110);
        REQUIRE(countEvents(events, CombatEventType::HeroDamaged) == 1U);
    }

    {
        BoxBoard board = test::solidBoard();
        Hero* attackerA = placeHero(board, 260, PieceColor::Red, {5, 0}, 10, 0);
        Hero* attackerB = placeHero(board, 261, PieceColor::Red, {5, 1}, 10, 0);
        Hero* victim = placeHero(board, 262, PieceColor::Yellow, {4, 0}, 10, 1);
        lowerToHp(*victim, 30);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(31U);
        const CombatResolution result = CombatResolver(board, towers, random).resolve(0);
        const CombatEvent* heroDamage = firstEvent(result.events, CombatEventType::HeroDamaged);
        REQUIRE(heroDamage != nullptr);
        REQUIRE(heroDamage->attackerHeroId == attackerA->id());
        REQUIRE(heroDamage->hpDamageApplied == 30);
        REQUIRE(heroDamage->overkillDamage == 110);
        REQUIRE(towers[1].currentHp() == Tower::MaxHp - 140);
        const CombatEvent* towerDamage = firstEvent(result.events, CombatEventType::TowerDamaged);
        REQUIRE(towerDamage != nullptr);
        REQUIRE(towerDamage->attackerHeroId == attackerB->id());
    }
}

void resolverStatusDeathAndAtomicTests() {
    {
        BoxBoard board = test::solidBoard();
        placeHero(board, 90, PieceColor::Red, {5, 0}, 1, 0);
        Hero* target = placeHero(board, 91, PieceColor::Green, {4, 0}, 10, 1);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(7U);
        const CombatResolution result = CombatResolver(board, towers, random).resolve(0);
        REQUIRE(target->vulnerableLayers() == 2);
        REQUIRE(countEvents(result.events, CombatEventType::VulnerableApplied) == 1U);
        REQUIRE(countEvents(result.events, CombatEventType::VulnerableReduced) == 1U);
    }
    {
        BoxBoard board = test::solidBoard();
        placeHero(board, 92, PieceColor::Yellow, {5, 0}, 1, 0);
        Hero* target = placeHero(board, 93, PieceColor::Green, {4, 0}, 10, 1);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(8U);
        const CombatResolution result = CombatResolver(board, towers, random).resolve(0);
        REQUIRE(target->vulnerableLayers() == 0);
        REQUIRE(target->weakLayers() == 1);
        REQUIRE(countEvents(result.events, CombatEventType::VulnerableApplied) == 1U);
        REQUIRE(countEvents(result.events, CombatEventType::VulnerableExpired) == 1U);
        REQUIRE(countEvents(result.events, CombatEventType::WeakApplied) == 1U);
        REQUIRE(countEvents(result.events, CombatEventType::WeakReduced) == 1U);
    }
    {
        BoxBoard board = test::solidBoard();
        Hero* hunter = placeHero(board, 100, PieceColor::Green, {5, 0}, 2, 0);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        static_cast<void>(towers[1].takeDamage(Tower::MaxHp - 1));
        CombatRandom random(9U);
        const CombatResolution result = CombatResolver(board, towers, random).resolve(0);
        REQUIRE(result.gameFinished);
        REQUIRE(result.winnerPlayerId == 0);
        REQUIRE(hunter->shield() == 16);
        REQUIRE(countEvents(result.events, CombatEventType::TowerDestroyed) == 1U);
        REQUIRE(countEvents(result.events, CombatEventType::ShieldGained) == 1U);
    }
    {
        BoxBoard board = test::solidBoard();
        placeHero(board, 110, PieceColor::Red, {5, 0}, 2, 0);
        Hero* enemy = placeHero(board, 111, PieceColor::Green, {4, 0}, 1, 1);
        lowerToHp(*enemy, 1);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(11U);
        const CombatResolution result = CombatResolver(board, towers, random).resolve(0);
        REQUIRE(countEvents(result.events, CombatEventType::HeroDied) == 1U);
        REQUIRE(countEvents(result.events, CombatEventType::HeroConvertedToBox) == 1U);
        REQUIRE(countEvents(result.events, CombatEventType::VulnerableApplied) == 0U);
        REQUIRE(board.pieceAt({4, 0})->type() == PieceType::Box);
    }
}

void statusExpiresAtEachPlayerTurnBoundary() {
    BoxBoard board = test::solidBoard();
    Hero* statusHero = placeHero(board, 135, PieceColor::Red, {5, 0}, 1, 0);
    statusHero->addVulnerableLayers(2);
    statusHero->addWeakLayers(2);
    std::array<Tower, 2> towers{Tower{0}, Tower{1}};
    CombatRandom random(13U);

    const CombatResolution playerOneTurn = CombatResolver(board, towers, random).resolve(1);
    REQUIRE(!playerOneTurn.gameFinished);
    REQUIRE(statusHero->vulnerableLayers() == 1);
    REQUIRE(statusHero->weakLayers() == 1);

    placeHero(board, 136, PieceColor::Yellow, {4, 0}, 10, 1);
    const CombatResolution playerZeroTurn = CombatResolver(board, towers, random).resolve(0);
    const CombatEvent* damage = firstEvent(playerZeroTurn.events, CombatEventType::HeroDamaged);
    REQUIRE(damage != nullptr);
    REQUIRE(damage->baseDamage == 14);
    REQUIRE(damage->damageAfterWeak == 11);
    REQUIRE(statusHero->vulnerableLayers() == 0);
    REQUIRE(statusHero->weakLayers() == 0);
}

void remainingAtomicActionTests() {
    {
        BoxBoard board = test::solidBoard();
        Hero* fighter = placeHero(board, 140, PieceColor::Red, {5, 0}, 1, 0);
        lowerToHp(*fighter, 10);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        static_cast<void>(towers[1].takeDamage(Tower::MaxHp - 1));
        CombatRandom random(1U);
        std::vector<CombatEvent> events;
        performOne(*fighter, 0, board, towers, random, events);
        REQUIRE(towers[1].isDestroyed());
        REQUIRE(fighter->currentHp() == 12);
        REQUIRE(countEvents(events, CombatEventType::VulnerableApplied) == 0U);
    }
    {
        BoxBoard board = test::solidBoard();
        auto* regent = dynamic_cast<Regent*>(placeHero(board, 141, PieceColor::Yellow, {5, 0}, 1, 0));
        REQUIRE(regent != nullptr);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        CombatRandom random(2U);
        std::vector<CombatEvent> events;
        performOne(*regent, 0, board, towers, random, events);
        REQUIRE(regent->radiantStars() == 1);
        static_cast<void>(towers[1].takeDamage(towers[1].currentHp() - 1));
        performOne(*regent, 0, board, towers, random, events);
        REQUIRE(towers[1].isDestroyed());
        REQUIRE(regent->radiantStars() == 4);
        REQUIRE(regent->shield() == 5);
    }
    {
        BoxBoard board = test::solidBoard();
        Hero* chicken = placeHero(board, 142, PieceColor::Blue, {5, 0}, 1, 0);
        placeHero(board, 143, PieceColor::Red, {4, 0}, 10, 1);
        std::array<Tower, 2> towers{Tower{0}, Tower{1}};
        static_cast<void>(towers[1].takeDamage(Tower::MaxHp - 1));
        CombatRandom random(std::vector<std::size_t>{1, 0});
        std::vector<CombatEvent> events;
        performOne(*chicken, 0, board, towers, random, events);
        REQUIRE(towers[1].isDestroyed());
        REQUIRE(chicken->lightningOrbs() == 0);
        REQUIRE(countEvents(events, CombatEventType::LightningActivated) == 1U);
    }
}

void movementAndRandomReproducibilityTests() {
    BoxBoard board = test::solidBoard();
    auto* regent = dynamic_cast<Regent*>(placeHero(board, 120, PieceColor::Yellow, {4, 2}, 2, 1));
    REQUIRE(regent != nullptr);
    REQUIRE(regent->gainShield(7) == 7);
    regent->addWeakLayers(2);
    regent->addVulnerableLayers(3);
    std::array<Tower, 2> towers{Tower{0}, Tower{1}};
    CombatRandom random(std::vector<std::size_t>{0});
    std::vector<CombatEvent> events;
    performOne(*regent, 1, board, towers, random, events);
    const int stars = regent->radiantStars();
    regent->setPosition({5, 2});
    REQUIRE(playerIdForPosition(regent->position()) == 0);
    REQUIRE(regent->shield() == 7);
    REQUIRE(regent->weakLayers() == 2);
    REQUIRE(regent->vulnerableLayers() == 3);
    REQUIRE(regent->radiantStars() == stars);

    auto resolveWithSeed = [](std::uint32_t seed) {
        BoxBoard randomBoard = test::solidBoard();
        placeHero(randomBoard, 130, PieceColor::Blue, {5, 0}, 2, 0);
        placeHero(randomBoard, 131, PieceColor::Red, {4, 0}, 10, 1);
        placeHero(randomBoard, 132, PieceColor::Green, {4, 1}, 10, 1);
        std::array<Tower, 2> randomTowers{Tower{0}, Tower{1}};
        CombatRandom seeded(seed);
        CombatResolution result = CombatResolver(randomBoard, randomTowers, seeded).resolve(0);
        return std::pair{std::move(result.events), randomBoard.snapshot()};
    };
    const auto first = resolveWithSeed(1234U);
    const auto second = resolveWithSeed(1234U);
    REQUIRE(first.first == second.first);
    REQUIRE(first.second == second.second);
}

void finishedSessionTests() {
    GameSession game = test::readySession();
    for (int row = 0; row < BoardRows; ++row) {
        for (int column = 0; column < BoardColumns; ++column) {
            game.boardForSetup().setPieceAt({row, column},
                std::make_unique<Box>(PieceColor::Red, Position{row, column}));
        }
    }
    const EliminateResult opening = game.handleEliminateRequest({0, 1, {{5, 0}, {5, 1}, {6, 1}}});
    REQUIRE(opening.success);
    REQUIRE(!opening.game.openingTurnPending);
    for (const HeroId id : game.board().livingHeroIds()) {
        const Hero* hero = game.board().heroById(id);
        REQUIRE(hero != nullptr);
        game.boardForSetup().setPieceAt(hero->position(),
            std::make_unique<Box>(PieceColor::Red, hero->position()));
    }
    static_cast<void>(game.towersForSetup()[0].takeDamage(999));
    game.boardForSetup().setPieceAt({0, 0}, std::make_unique<Box>(PieceColor::Red, Position{0, 0}));
    game.boardForSetup().setPieceAt({0, 1}, std::make_unique<Box>(PieceColor::Red, Position{0, 1}));
    game.boardForSetup().setPieceAt({1, 1}, std::make_unique<Box>(PieceColor::Red, Position{1, 1}));
    const EliminateResult result = game.handleEliminateRequest({1, 2, {{0, 0}, {0, 1}, {1, 1}}});
    REQUIRE(result.success);
    REQUIRE(game.phase() == GamePhase::Finished);
    REQUIRE(game.winnerPlayerId() == 1);
    REQUIRE(game.currentPlayerId() == 1);
    REQUIRE(game.turnId() == 2);
}

} // namespace

void runCombatTests() {
    positionAttributeAndResetTests();
    attackOrderTests();
    ironFighterTests();
    silentHunterTests();
    regentTests();
    chickenPotTests();
    singleTargetAndOverkillTests();
    resolverStatusDeathAndAtomicTests();
    statusExpiresAtEachPlayerTurnBoundary();
    remainingAtomicActionTests();
    movementAndRandomReproducibilityTests();
    finishedSessionTests();
}
