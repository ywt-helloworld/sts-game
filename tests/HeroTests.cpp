#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

using namespace sts;

void runHeroTests() {
    auto attacker = HeroFactory::create(PieceColor::Red, {0, 0}, 20);
    auto target = HeroFactory::create(PieceColor::Blue, {0, 1}, 4);
    attacker->attack(*target);
    REQUIRE(target->isDead());
    REQUIRE(target->inheritedAttribute() == 0);

    GameSession game = test::readySession();
    auto& board = game.boardForSetup();
    for (int row = 0; row < BoxBoard::RowCount; ++row) {
        for (int column = 0; column < BoxBoard::ColumnCount; ++column) {
            board.setPieceAt({row, column}, std::make_unique<Box>(PieceColor::Red, Position{row, column}));
        }
    }
    const EliminateResult rejectedPlayer = game.handleEliminateRequest({1, 1, {{0, 0}, {0, 1}, {1, 1}}});
    REQUIRE(!rejectedPlayer.success);
    REQUIRE(game.currentPlayerId() == 0);
    REQUIRE(game.turnId() == 1);
    const EliminateResult rejectedTurn = game.handleEliminateRequest({0, 0, {{5, 0}, {5, 1}, {6, 1}}});
    REQUIRE(!rejectedTurn.success);
    REQUIRE(game.currentPlayerId() == 0);
    REQUIRE(game.turnId() == 1);
    const EliminateResult accepted = game.handleEliminateRequest({0, 1, {{5, 0}, {5, 1}, {6, 1}}});
    REQUIRE(accepted.success);
    REQUIRE(accepted.nextPlayerId == 1);
    REQUIRE(accepted.nextTurnId == 2);
}
