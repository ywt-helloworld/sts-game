#pragma once

#include "common/PieceColor.hpp"
#include "common/Position.hpp"

namespace sts {

class BoardPiece {
public:
    BoardPiece(PieceColor color, Position position) : color_(color), position_(position) {}
    virtual ~BoardPiece() = default;

    [[nodiscard]] PieceColor color() const noexcept { return color_; }
    [[nodiscard]] Position position() const noexcept { return position_; }
    void setPosition(Position position) noexcept { position_ = position; }

    [[nodiscard]] virtual PieceType type() const noexcept = 0;
    [[nodiscard]] virtual int inheritedAttribute() const noexcept = 0;
    virtual void onSpawn() = 0;
    virtual void onEliminated() = 0;

protected:
    PieceColor color_;
    Position position_;
};

} // namespace sts
