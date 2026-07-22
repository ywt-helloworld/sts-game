#include "game/Box.hpp"

#include <array>
#include <cstdlib>
#include <memory>

namespace sts {
namespace {

const std::shared_ptr<const ImageResource>& imageFor(PieceColor color) {
    static const std::array<std::shared_ptr<const ImageResource>, 4> images{
        std::make_shared<const ImageResource>("box-red"),
        std::make_shared<const ImageResource>("box-yellow"),
        std::make_shared<const ImageResource>("box-green"),
        std::make_shared<const ImageResource>("box-blue")};
    return images.at(static_cast<std::size_t>(color));
}

} // namespace

Box::Box(PieceColor color, Position position) : BoardPiece(color, position), image_(imageFor(color)) {
    onSpawn();
}

void Box::onSpawn() {
    active_ = true;
    ++spawnEvents_;
}

void Box::onEliminated() {
    active_ = false;
    ++eliminationEvents_;
}

bool Box::canConnectTo(const BoardPiece& other) const noexcept {
    const Position there = other.position();
    return color() == other.color() && std::abs(position().row - there.row) <= 1 &&
           std::abs(position().column - there.column) <= 1 && position() != there;
}

} // namespace sts
