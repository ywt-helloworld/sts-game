#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

#include "common/BoardDimensions.hpp"
#include "common/PlayerViewTransform.hpp"
#include "game/EliminationRule.hpp"

#include <cmath>
#include <vector>

using namespace sts;

namespace {

bool areEightNeighbors(Position first, Position second) {
    return first != second && std::abs(first.row - second.row) <= 1 &&
           std::abs(first.column - second.column) <= 1;
}

} // namespace

void runViewTransformTests() {
    BoxBoard board = test::solidBoard();
    REQUIRE(BoardRows == 10);
    REQUIRE(BoardColumns == 5);
    REQUIRE(board.isInside({9, 4}));
    REQUIRE(!board.isInside({10, 4}));
    REQUIRE(!board.isInside({9, 5}));

    const BoardSnapshot snapshot = board.snapshot();
    int snapshotPositions{};
    for (const auto& row : snapshot) {
        for (const PieceSnapshot& piece : row) {
            ++snapshotPositions;
            REQUIRE(board.isInside(piece.position));
        }
    }
    REQUIRE(snapshotPositions == BoardRows * BoardColumns);

    const Position player0Position{7, 2};
    REQUIRE(PlayerViewTransform::logicalToDisplay(0, player0Position) == player0Position);
    REQUIRE(PlayerViewTransform::displayToLogical(0, player0Position) == player0Position);

    for (int row = 0; row < BoardRows; ++row) {
        for (int column = 0; column < BoardColumns; ++column) {
            const Position logical{row, column};
            const Position display = PlayerViewTransform::logicalToDisplay(1, logical);
            REQUIRE(PlayerViewTransform::displayToLogical(1, display) == logical);
        }
    }
    const Position player1TopLeftDisplay{9, 4};
    const Position player1TopRightDisplay{5, 0};
    REQUIRE(PlayerViewTransform::logicalToDisplay(1, {0, 0}) == player1TopLeftDisplay);
    REQUIRE(PlayerViewTransform::logicalToDisplay(1, {4, 4}) == player1TopRightDisplay);
    REQUIRE(!PlayerViewTransform::isOwnDisplayArea({4, 0}));
    REQUIRE(PlayerViewTransform::isOwnDisplayArea({5, 0}));
    REQUIRE(PlayerViewTransform::isOwnDisplayArea({9, 4}));

    const Position first{2, 1};
    const Position second{3, 2};
    REQUIRE(areEightNeighbors(first, second));
    REQUIRE(areEightNeighbors(PlayerViewTransform::logicalToDisplay(1, first),
                               PlayerViewTransform::logicalToDisplay(1, second)));

    const std::vector<Position> player1DisplayPath{{5, 4}, {5, 3}, {6, 3}};
    std::vector<Position> player1LogicalPath;
    for (const Position display : player1DisplayPath) {
        player1LogicalPath.push_back(PlayerViewTransform::displayToLogical(1, display));
    }
    REQUIRE(player1LogicalPath == std::vector<Position>({{4, 0}, {4, 1}, {3, 1}}));
    EliminationRule rule;
    REQUIRE(rule.validate(board, 1, player1LogicalPath).valid);

    // Each view indexes the same snapshot through its transform; it never owns a copy.
    const Position player0Display{9, 4};
    const Position player1Display{0, 0};
    const Position expectedLogical{9, 4};
    const Position player0Logical = PlayerViewTransform::displayToLogical(0, player0Display);
    const Position player1Logical = PlayerViewTransform::displayToLogical(1, player1Display);
    REQUIRE(snapshot[player0Logical.row][player0Logical.column].position == expectedLogical);
    REQUIRE(snapshot[player1Logical.row][player1Logical.column].position == expectedLogical);
}
