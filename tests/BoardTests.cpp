#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

#include "common/PlayerViewTransform.hpp"
#include "game/Hero.hpp"

using namespace sts;

namespace {

void requireFullAndConsistent(const BoxBoard& board) {
    const BoardSnapshot snapshot = board.snapshot();
    for (int row = 0; row < BoxBoard::RowCount; ++row) {
        for (int column = 0; column < BoxBoard::ColumnCount; ++column) {
            const Position expected{row, column};
            REQUIRE(board.pieceAt(expected) != nullptr);
            REQUIRE(snapshot[row][column].position == expected);
        }
    }
}

void requireColumnUnchanged(const BoardSnapshot& before, const BoardSnapshot& after, int column) {
    for (int row = 0; row < BoxBoard::RowCount; ++row) {
        REQUIRE(before[row][column] == after[row][column]);
    }
}

void testPlayerZeroCollapsesDownAndMovesHeroes() {
    BoxBoard board = test::solidBoard();
    board.setPieceAt({5, 2}, HeroFactory::create(100, PieceColor::Red, {5, 2}, 50));
    BoardPiece* firstSurvivor = board.pieceAt({0, 2});
    BoardPiece* lastSurvivor = board.pieceAt({6, 2});
    BoardPiece* existingHero = board.pieceAt({5, 2});
    const BoardSnapshot before = board.snapshot();

    static_cast<void>(board.resolveElimination(0, {{7, 2}, {8, 2}, {9, 2}}));
    const BoardSnapshot after = board.snapshot();

    REQUIRE(board.pieceAt({2, 2}) == firstSurvivor);
    REQUIRE(board.pieceAt({8, 2}) == lastSurvivor);
    REQUIRE(board.pieceAt({7, 2}) == existingHero);
    const auto* movedHero = dynamic_cast<const RedHero*>(board.pieceAt({7, 2}));
    REQUIRE(movedHero != nullptr);
    REQUIRE(movedHero->inheritedAttribute() == 50);
    REQUIRE(movedHero->attackPower() == 700);
    REQUIRE(movedHero->defense() == 0);
    const Position movedHeroPosition{7, 2};
    REQUIRE(movedHero->position() == movedHeroPosition);

    const auto* generatedHero = dynamic_cast<const RedHero*>(board.pieceAt({9, 2}));
    REQUIRE(generatedHero != nullptr);
    REQUIRE(generatedHero->inheritedAttribute() == 3);
    REQUIRE(board.pieceAt({0, 2})->type() == PieceType::Box);
    REQUIRE(board.pieceAt({1, 2})->type() == PieceType::Box);
    requireColumnUnchanged(before, after, 4);
    requireFullAndConsistent(board);
}

void testPlayerOneCollapsesUpAndMovesHeroes() {
    BoxBoard board = test::solidBoard(PieceColor::Blue);
    board.setPieceAt({4, 3}, HeroFactory::create(200, PieceColor::Blue, {4, 3}, 44));
    BoardPiece* existingHero = board.pieceAt({4, 3});
    BoardPiece* survivorAboveHero = board.pieceAt({3, 3});
    BoardPiece* survivorBelowHero = board.pieceAt({5, 3});
    const BoardSnapshot before = board.snapshot();

    static_cast<void>(board.resolveElimination(1, {{0, 3}, {1, 3}, {2, 3}}));
    const BoardSnapshot after = board.snapshot();

    const auto* generatedHero = dynamic_cast<const BlueHero*>(board.pieceAt({0, 3}));
    REQUIRE(generatedHero != nullptr);
    REQUIRE(generatedHero->inheritedAttribute() == 3);
    REQUIRE(board.pieceAt({2, 3}) == existingHero);
    const auto* movedHero = dynamic_cast<const BlueHero*>(board.pieceAt({2, 3}));
    REQUIRE(movedHero != nullptr);
    REQUIRE(movedHero->inheritedAttribute() == 44);
    REQUIRE(movedHero->attackPower() == 264);
    REQUIRE(movedHero->defense() == 0);
    const Position beforeHeroDisplay = PlayerViewTransform::logicalToDisplay(1, {4, 3});
    const Position afterHeroDisplay = PlayerViewTransform::logicalToDisplay(1, movedHero->position());
    REQUIRE(afterHeroDisplay.row > beforeHeroDisplay.row);
    REQUIRE(board.pieceAt({1, 3}) == survivorAboveHero);
    REQUIRE(board.pieceAt({3, 3}) == survivorBelowHero);
    REQUIRE(board.pieceAt({8, 3})->type() == PieceType::Box);
    REQUIRE(board.pieceAt({9, 3})->type() == PieceType::Box);
    requireColumnUnchanged(before, after, 0);
    requireFullAndConsistent(board);
}

void testMultipleAffectedColumnsAndNonContiguousGaps() {
    BoxBoard board = test::solidBoard();
    BoardPiece* firstColumnZeroSurvivor = board.pieceAt({0, 0});
    BoardPiece* firstColumnOneSurvivor = board.pieceAt({0, 1});
    const BoardSnapshot before = board.snapshot();

    // This legal, zig-zag path removes rows 5, 6 and 8 from column 0,
    // leaving non-contiguous gaps in that column, and also affects column 1.
    static_cast<void>(board.resolveElimination(0, {{5, 0}, {5, 1}, {6, 0}, {7, 1}, {8, 0}}));
    const BoardSnapshot after = board.snapshot();

    REQUIRE(after[0][0].type == PieceType::Box);
    REQUIRE(after[1][0].type == PieceType::Box);
    REQUIRE(after[2][0].type == PieceType::Box);
    REQUIRE(board.pieceAt({2, 0}) == firstColumnZeroSurvivor);
    REQUIRE(board.pieceAt({2, 1}) == firstColumnOneSurvivor);
    requireColumnUnchanged(before, after, 2);
    requireColumnUnchanged(before, after, 3);
    requireColumnUnchanged(before, after, 4);
    requireFullAndConsistent(board);
}

} // namespace

void runBoardTests() {
    testPlayerZeroCollapsesDownAndMovesHeroes();
    testPlayerOneCollapsesUpAndMovesHeroes();
    testMultipleAffectedColumnsAndNonContiguousGaps();
}
