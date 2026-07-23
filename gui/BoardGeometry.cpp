#include "gui/BoardGeometry.hpp"

#include <algorithm>

namespace sts {

BoardGeometry BoardGeometry::centered(float windowWidth, float windowHeight) noexcept {
    constexpr float horizontalMargin = 180.0F;
    constexpr float boardHeightRatio = 0.90F;
    const float availableWidth = std::max(0.0F, windowWidth - horizontalMargin * 2.0F);
    const float availableHeight = std::max(0.0F, windowHeight * boardHeightRatio);
    const float cell = std::max(1.0F, std::min(availableWidth / static_cast<float>(BoardColumns),
                                               availableHeight / static_cast<float>(BoardRows)));
    const float width = cell * static_cast<float>(BoardColumns);
    const float height = cell * static_cast<float>(BoardRows);
    return {(windowWidth - width) / 2.0F, (windowHeight - height) / 2.0F, cell};
}

std::optional<Position> BoardGeometry::displayPositionAt(float x, float y) const noexcept {
    if (cellSize <= 0.0F || x < left || y < top || x >= left + boardWidth() || y >= top + boardHeight()) {
        return std::nullopt;
    }
    const Position position{static_cast<int>((y - top) / cellSize), static_cast<int>((x - left) / cellSize)};
    if (position.row < 0 || position.row >= BoardRows || position.column < 0 || position.column >= BoardColumns) {
        return std::nullopt;
    }
    return position;
}

} // namespace sts
