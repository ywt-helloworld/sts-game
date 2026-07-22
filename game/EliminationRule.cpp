#include "game/EliminationRule.hpp"

#include "game/BoxBoard.hpp"

#include <cmath>
#include <set>

namespace sts {

ValidationResult EliminationRule::validate(const BoxBoard& board, int playerId,
                                           const std::vector<Position>& path) const {
    if (playerId != 0 && playerId != 1) {
        return {false, "unknown player"};
    }
    if (path.size() < 3U) {
        return {false, "path must contain at least 3 positions"};
    }

    std::set<Position> seen;
    const BoardPiece* previous = nullptr;
    for (const Position position : path) {
        if (!board.isInside(position)) {
            return {false, "path contains an out-of-bounds position"};
        }
        if (!seen.insert(position).second) {
            return {false, "path contains a duplicate position"};
        }
        if (!isInPlayerArea(playerId, position)) {
            return {false, "path contains a position outside the player's area"};
        }
        const BoardPiece* current = board.pieceAt(position);
        if (current == nullptr) {
            return {false, "path contains an empty board position"};
        }
        if (previous != nullptr) {
            if (previous->color() != current->color()) {
                return {false, "all pieces in the path must have the same color"};
            }
            const Position before = previous->position();
            if (std::abs(before.row - position.row) > 1 || std::abs(before.column - position.column) > 1) {
                return {false, "successive path positions must be adjacent"};
            }
        }
        previous = current;
    }
    return {true, {}};
}

bool EliminationRule::isInPlayerArea(int playerId, Position position) noexcept {
    if (position.column < 0 || position.column >= BoxBoard::ColumnCount) {
        return false;
    }
    if (playerId == 0) {
        return position.row >= PlayerAreaRows && position.row < BoxBoard::RowCount;
    }
    return position.row >= 0 && position.row < PlayerAreaRows;
}

} // namespace sts
