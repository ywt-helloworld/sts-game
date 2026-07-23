#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

#include "game/EliminationRule.hpp"

using namespace sts;

void runEliminationRuleTests() {
    EliminationRule rule;
    BoxBoard board = test::solidBoard();

    REQUIRE(!rule.validate(board, 0, {{5, 0}, {5, 1}}).valid);

    board.setPieceAt({5, 1}, std::make_unique<Box>(PieceColor::Blue, Position{5, 1}));
    REQUIRE(!rule.validate(board, 0, {{5, 0}, {5, 1}, {6, 1}}).valid);
    board.setPieceAt({5, 1}, std::make_unique<Box>(PieceColor::Red, Position{5, 1}));

    REQUIRE(!rule.validate(board, 0, {{5, 0}, {5, 2}, {5, 3}}).valid);
    REQUIRE(!rule.validate(board, 0, {{5, 0}, {5, 1}, {5, 0}}).valid);
    REQUIRE(!rule.validate(board, 0, {{5, 2}, {5, 3}, {4, 3}}).valid);

    board.setPieceAt({6, 1}, HeroFactory::create(300, PieceColor::Red, {6, 1}, 50));
    REQUIRE(rule.validate(board, 0, {{5, 0}, {5, 1}, {6, 1}}).valid);
    board.setPieceAt({6, 1}, HeroFactory::create(301, PieceColor::Blue, {6, 1}, 50));
    REQUIRE(!rule.validate(board, 0, {{5, 0}, {5, 1}, {6, 1}}).valid);

    REQUIRE(rule.validate(board, 1, {{0, 0}, {0, 1}, {1, 1}}).valid);
    REQUIRE(!rule.validate(board, 1, {{5, 0}, {5, 1}, {6, 1}}).valid);
}
