#include "game/GameSession.hpp"
#include "game/Box.hpp"

#include <iostream>
#include <memory>

namespace {

using namespace sts;

void printBoard(const BoardSnapshot& board) {
    for (const auto& row : board) {
        for (const PieceSnapshot& piece : row) {
            const char color = "RYGB"[static_cast<int>(piece.color)];
            std::cout << color << (piece.type == PieceType::Hero ? 'H' : 'B') << ' ';
        }
        std::cout << '\n';
    }
}

} // namespace

int main() {
    GameSession game(42);
    auto& board = game.boardForSetup();
    // Make a deterministic valid first move for the console demonstration.
    board.setPieceAt({5, 0}, std::make_unique<Box>(PieceColor::Red, Position{5, 0}));
    board.setPieceAt({5, 1}, std::make_unique<Box>(PieceColor::Red, Position{5, 1}));
    board.setPieceAt({6, 1}, std::make_unique<Box>(PieceColor::Red, Position{6, 1}));

    static_cast<void>(game.markPlayerReady(0));
    static_cast<void>(game.markPlayerReady(1));
    static_cast<void>(game.start());
    const EliminateResult result = game.handleEliminateRequest({0, 1, {{5, 0}, {5, 1}, {6, 1}}});

    std::cout << (result.success ? "Turn accepted" : "Turn rejected") << "\n";
    if (!result.success) {
        std::cout << result.errorMessage << '\n';
        return 1;
    }
    for (const auto& row : result.game.board) {
        for (const PieceSnapshot& piece : row) {
            if (piece.type == PieceType::Hero) {
                std::cout << "Hero at (" << piece.position.row << ", " << piece.position.column
                          << "): attribute=" << piece.attributeValue
                          << ", hp=" << piece.currentHp << '/' << piece.maxHp << "\n";
            }
        }
    }
    std::cout << "Next player=" << result.game.currentPlayerId << ", turn=" << result.game.turnId << "\n\n";
    printBoard(result.game.board);
}
