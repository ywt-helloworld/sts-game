#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

#include "game/Hero.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <vector>

namespace {

using namespace sts;

void fillBoard(BoxBoard& board, PieceColor color) {
    for (int row = 0; row < BoardRows; ++row) {
        for (int column = 0; column < BoardColumns; ++column) {
            board.setPieceAt({row, column}, std::make_unique<Box>(color, Position{row, column}));
        }
    }
}

GameSession readyOpeningSession(PieceColor color, int firstPlayerId = 0) {
    GameSession game(17U, 29U, firstPlayerId);
    fillBoard(game.boardForSetup(), color);
    REQUIRE(game.markPlayerReady(0));
    REQUIRE(game.markPlayerReady(1));
    REQUIRE(game.start());
    return game;
}

std::vector<Position> openingPathFor(int playerId) {
    return playerId == 0
               ? std::vector<Position>{{5, 0}, {5, 1}, {6, 1}}
               : std::vector<Position>{{4, 4}, {4, 3}, {3, 3}};
}

bool hasEvent(const EliminateResult& result, CombatEventType type) {
    return std::ranges::any_of(result.combatEvents, [type](const CombatEvent& event) {
        return event.type == type;
    });
}

void requireNoCombatEvents(const EliminateResult& result) {
    const std::array forbidden{
        CombatEventType::CombatStarted,
        CombatEventType::HeroAttacked,
        CombatEventType::HeroDamaged,
        CombatEventType::VulnerableApplied,
        CombatEventType::WeakApplied,
        CombatEventType::ShieldGained,
        CombatEventType::ShieldAbsorbed,
        CombatEventType::ShieldBroken,
        CombatEventType::HeroHealed,
        CombatEventType::RadiantStarsChanged,
        CombatEventType::RegentAttackModeSelected,
        CombatEventType::LightningAttackStarted,
        CombatEventType::LightningActivated,
        CombatEventType::LightningTargetSelected,
        CombatEventType::LightningOrbChanged,
        CombatEventType::HeroDied,
        CombatEventType::OverflowDamageGenerated,
        CombatEventType::TowerDamaged,
        CombatEventType::TowerDestroyed,
        CombatEventType::CombatFinished,
        CombatEventType::GameFinished,
    };
    for (const CombatEventType type : forbidden) {
        REQUIRE(!hasEvent(result, type));
    }
}

void openingCreatesEachHeroWithoutCombat() {
    const std::array colors{PieceColor::Red, PieceColor::Green, PieceColor::Yellow, PieceColor::Blue};
    for (const PieceColor color : colors) {
        GameSession game = readyOpeningSession(color);
        REQUIRE(game.openingTurnPending());
        REQUIRE(game.snapshot().openingTurnPending);
        REQUIRE(game.currentPlayerId() == 0);
        REQUIRE(game.turnId() == 1);

        const EliminateResult result = game.handleEliminateRequest({0, 1, openingPathFor(0)});
        REQUIRE(result.success);
        REQUIRE(result.game.phase == GamePhase::Elimination);
        REQUIRE(!result.game.openingTurnPending);
        REQUIRE(!game.openingTurnPending());
        REQUIRE(result.game.currentPlayerId == 1);
        REQUIRE(result.game.turnId == 2);
        REQUIRE(result.game.towers[0].currentHp == Tower::MaxHp);
        REQUIRE(result.game.towers[1].currentHp == Tower::MaxHp);
        REQUIRE(hasEvent(result, CombatEventType::OpeningTurnCompleted));
        REQUIRE(hasEvent(result, CombatEventType::TurnChanged));
        requireNoCombatEvents(result);

        const std::vector<HeroId> heroes = game.board().livingHeroIds();
        REQUIRE(heroes.size() == 1U);
        const Hero* hero = game.board().heroById(heroes.front());
        REQUIRE(hero != nullptr);
        REQUIRE(hero->attributeValue() == 3);
        REQUIRE(hero->currentHp() == hero->maxHp());
        REQUIRE(hero->defense() == 0);
        REQUIRE(hero->shield() == 0);
        REQUIRE(hero->vulnerableLayers() == 0);
        REQUIRE(hero->weakLayers() == 0);

        switch (color) {
        case PieceColor::Red:
            REQUIRE(hero->heroType() == HeroType::IronFighter);
            REQUIRE(hero->maxHp() == 48);
            break;
        case PieceColor::Green:
            REQUIRE(hero->heroType() == HeroType::SilentHunter);
            REQUIRE(hero->maxHp() == 42);
            break;
        case PieceColor::Yellow:
            REQUIRE(hero->heroType() == HeroType::Regent);
            REQUIRE(hero->maxHp() == 45);
            REQUIRE(hero->radiantStars() == Regent::InitialRadiantStars);
            break;
        case PieceColor::Blue:
            REQUIRE(hero->heroType() == HeroType::ChickenPot);
            REQUIRE(hero->maxHp() == 45);
            REQUIRE(hero->lightningOrbs() == ChickenPot::InitialLightningOrbs);
            break;
        }

        for (int row = 0; row < BoardRows; ++row) {
            for (int column = 0; column < BoardColumns; ++column) {
                REQUIRE(game.board().pieceAt({row, column}) != nullptr);
                REQUIRE(result.game.board[row][column].position == (Position{row, column}));
            }
        }
    }
}

void openingDoesNotDecayOrTriggerExistingHeroes() {
    GameSession game = readyOpeningSession(PieceColor::Red);
    auto existing = HeroFactory::create(100, PieceColor::Green, {9, 4}, 2, 0);
    existing->addVulnerableLayers(2);
    existing->addWeakLayers(2);
    static_cast<void>(existing->receiveDamage(5));
    REQUIRE(existing->gainShield(7) == 7);
    const int hp = existing->currentHp();
    game.boardForSetup().setPieceAt({9, 4}, std::move(existing));

    const EliminateResult result = game.handleEliminateRequest({0, 1, openingPathFor(0)});
    REQUIRE(result.success);
    const Hero* preserved = game.board().heroById(100);
    REQUIRE(preserved != nullptr);
    REQUIRE(preserved->currentHp() == hp);
    REQUIRE(preserved->shield() == 7);
    REQUIRE(preserved->vulnerableLayers() == 2);
    REQUIRE(preserved->weakLayers() == 2);
    REQUIRE(result.game.towers[1].currentHp == Tower::MaxHp);
    requireNoCombatEvents(result);
}

void firstPlayerCanBePlayerOne() {
    GameSession game = readyOpeningSession(PieceColor::Blue, 1);
    REQUIRE(game.currentPlayerId() == 1);
    REQUIRE(game.openingTurnPending());
    const EliminateResult result = game.handleEliminateRequest({1, 1, openingPathFor(1)});
    REQUIRE(result.success);
    REQUIRE(!result.game.openingTurnPending);
    REQUIRE(result.game.currentPlayerId == 0);
    REQUIRE(result.game.turnId == 2);
    REQUIRE(game.board().livingHeroIds().size() == 1U);
    requireNoCombatEvents(result);
}

void secondAndLaterTurnsUseNormalCombat() {
    GameSession game = readyOpeningSession(PieceColor::Red);
    const EliminateResult opening = game.handleEliminateRequest({0, 1, openingPathFor(0)});
    REQUIRE(opening.success);
    REQUIRE(!opening.game.openingTurnPending);

    game.boardForSetup().setPieceAt({4, 4}, std::make_unique<Box>(PieceColor::Red, Position{4, 4}));
    game.boardForSetup().setPieceAt({4, 3}, std::make_unique<Box>(PieceColor::Red, Position{4, 3}));
    game.boardForSetup().setPieceAt({3, 3}, std::make_unique<Box>(PieceColor::Red, Position{3, 3}));
    const EliminateResult second = game.handleEliminateRequest({1, 2, openingPathFor(1)});
    REQUIRE(second.success);
    REQUIRE(hasEvent(second, CombatEventType::CombatStarted));
    REQUIRE(hasEvent(second, CombatEventType::CombatFinished));
    REQUIRE(!hasEvent(second, CombatEventType::OpeningTurnCompleted));
    const auto newHeroActed = std::ranges::find_if(second.combatEvents, [](const CombatEvent& event) {
        return event.attackerHeroId == HeroId{2} &&
               (event.type == CombatEventType::HeroAttacked || event.type == CombatEventType::TowerDamaged);
    });
    REQUIRE(newHeroActed != second.combatEvents.end());
    REQUIRE(second.game.currentPlayerId == 0);
    REQUIRE(second.game.turnId == 3);

    const std::vector<Position> thirdPath{{8, 3}, {8, 4}, {9, 4}};
    for (const Position position : thirdPath) {
        game.boardForSetup().setPieceAt(position, std::make_unique<Box>(PieceColor::Red, position));
    }
    const EliminateResult third = game.handleEliminateRequest({0, 3, thirdPath});
    REQUIRE(third.success);
    REQUIRE(hasEvent(third, CombatEventType::CombatStarted));
    REQUIRE(hasEvent(third, CombatEventType::CombatFinished));
    REQUIRE(!third.game.openingTurnPending);
    REQUIRE(third.game.currentPlayerId == 1);
    REQUIRE(third.game.turnId == 4);
}

} // namespace

void runOpeningTurnTests() {
    openingCreatesEachHeroWithoutCombat();
    openingDoesNotDecayOrTriggerExistingHeroes();
    firstPlayerCanBePlayerOne();
    secondAndLaterTurnsUseNormalCombat();
}
