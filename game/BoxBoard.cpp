#include "game/BoxBoard.hpp"

#include "game/Box.hpp"
#include "game/HeroFactory.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sts {

BoxBoard::BoxBoard() : BoxBoard(std::random_device{}()) {}

BoxBoard::BoxBoard(std::uint32_t seed) : random_(seed) {
    initializeRandom();
}

void BoxBoard::initializeRandom() {
    for (int row = 0; row < RowCount; ++row) {
        for (int column = 0; column < ColumnCount; ++column) {
            const Position position{row, column};
            cells_[row][column] = generateBoxAt(position);
        }
    }
}

const BoardPiece* BoxBoard::pieceAt(Position position) const noexcept {
    if (!isInside(position)) {
        return nullptr;
    }
    return cells_[position.row][position.column].get();
}

BoardPiece* BoxBoard::pieceAt(Position position) noexcept {
    return const_cast<BoardPiece*>(std::as_const(*this).pieceAt(position));
}

bool BoxBoard::isInside(Position position) const noexcept {
    return position.row >= 0 && position.row < RowCount && position.column >= 0 && position.column < ColumnCount;
}

BoardSnapshot BoxBoard::snapshot() const {
    BoardSnapshot result{};
    for (int row = 0; row < RowCount; ++row) {
        for (int column = 0; column < ColumnCount; ++column) {
            const Position position{row, column};
            const BoardPiece* piece = pieceAt(position);
            if (piece == nullptr) {
                throw std::logic_error("a board snapshot cannot contain an empty cell");
            }
            result[row][column] = PieceSnapshot{piece->type(), piece->color(), piece->position(), piece->inheritedAttribute()};
        }
    }
    return result;
}

void BoxBoard::setPieceAt(Position position, std::unique_ptr<BoardPiece> piece) {
    if (!isInside(position) || piece == nullptr) {
        throw std::invalid_argument("setPieceAt needs an in-board, non-null piece");
    }
    piece->setPosition(position);
    cells_[position.row][position.column] = std::move(piece);
}

std::unique_ptr<BoardPiece> BoxBoard::generateBoxAt(Position position) {
    std::uniform_int_distribution<int> colorDistribution(0, 3);
    return std::make_unique<Box>(static_cast<PieceColor>(colorDistribution(random_)), position);
}

void BoxBoard::resolveElimination(int actingPlayerId, const std::vector<Position>& path) {
    if (actingPlayerId != 0 && actingPlayerId != 1) {
        throw std::invalid_argument("actingPlayerId must be 0 or 1");
    }
    if (path.empty()) {
        throw std::invalid_argument("cannot resolve an empty elimination path");
    }
    const Position destination = path.back();
    const BoardPiece* finalPiece = pieceAt(destination);
    if (finalPiece == nullptr) {
        throw std::logic_error("elimination path contains an empty position");
    }

    const PieceColor color = finalPiece->color();
    int attribute = static_cast<int>(path.size()) * 10;
    for (const Position position : path) {
        BoardPiece* piece = pieceAt(position);
        if (piece == nullptr) {
            throw std::logic_error("elimination path contains an empty position");
        }
        attribute += piece->inheritedAttribute();
        piece->onEliminated();
    }
    for (const Position position : path) {
        cells_[position.row][position.column].reset();
    }
    setPieceAt(destination, HeroFactory::create(color, destination, attribute));

    // The new hero is deliberately not an anchor. It joins every other
    // BoardPiece in the direction selected by the acting player's logical side.
    collapseAndRefill(actingPlayerId, path);
}

void BoxBoard::collapseAndRefill(int actingPlayerId, const std::vector<Position>& eliminatedPath) {
    if (actingPlayerId != 0 && actingPlayerId != 1) {
        throw std::invalid_argument("actingPlayerId must be 0 or 1");
    }
    std::array<bool, ColumnCount> affectedColumns{};
    for (const Position position : eliminatedPath) {
        if (!isInside(position)) {
            throw std::invalid_argument("elimination path contains an out-of-bounds position");
        }
        affectedColumns[static_cast<std::size_t>(position.column)] = true;
    }
    for (int column = 0; column < ColumnCount; ++column) {
        if (!affectedColumns[static_cast<std::size_t>(column)]) {
            continue;
        }
        if (actingPlayerId == 0) {
            collapseColumnDown(column);
        } else {
            collapseColumnUp(column);
        }
    }
}

void BoxBoard::collapseColumnDown(int column) {
    int writeRow = RowCount - 1;
    for (int readRow = RowCount - 1; readRow >= 0; --readRow) {
        if (cells_[readRow][column] == nullptr) {
            continue;
        }
        if (readRow != writeRow) {
            cells_[writeRow][column] = std::move(cells_[readRow][column]);
        }
        cells_[writeRow][column]->setPosition(Position{writeRow, column});
        --writeRow;
    }
    while (writeRow >= 0) {
        const Position position{writeRow, column};
        cells_[writeRow][column] = generateBoxAt(position);
        --writeRow;
    }
}

void BoxBoard::collapseColumnUp(int column) {
    int writeRow = 0;
    for (int readRow = 0; readRow < RowCount; ++readRow) {
        if (cells_[readRow][column] == nullptr) {
            continue;
        }
        if (readRow != writeRow) {
            cells_[writeRow][column] = std::move(cells_[readRow][column]);
        }
        cells_[writeRow][column]->setPosition(Position{writeRow, column});
        ++writeRow;
    }
    while (writeRow < RowCount) {
        const Position position{writeRow, column};
        cells_[writeRow][column] = generateBoxAt(position);
        ++writeRow;
    }
}

} // namespace sts
