#pragma once

#include "game/Box.hpp"
#include "game/BoxBoard.hpp"
#include "game/GameSession.hpp"
#include "game/HeroFactory.hpp"

#include <memory>

namespace sts::test {

inline BoxBoard solidBoard(PieceColor color = PieceColor::Red) {
    BoxBoard board(7);
    for (int row = 0; row < BoxBoard::RowCount; ++row) {
        for (int column = 0; column < BoxBoard::ColumnCount; ++column) {
            board.setPieceAt({row, column}, std::make_unique<Box>(color, Position{row, column}));
        }
    }
    return board;
}

inline GameSession readySession(std::uint32_t seed = 7) {
    GameSession game(seed);
    static_cast<void>(game.markPlayerReady(0));
    static_cast<void>(game.markPlayerReady(1));
    static_cast<void>(game.start());
    return game;
}

} // namespace sts::test
