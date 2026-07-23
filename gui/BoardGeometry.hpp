#pragma once

#include "common/BoardDimensions.hpp"
#include "common/Position.hpp"

#include <optional>

namespace sts {

struct BoardGeometry {
    float left{};
    float top{};
    float cellSize{};

    [[nodiscard]] static BoardGeometry centered(float windowWidth, float windowHeight) noexcept;
    [[nodiscard]] std::optional<Position> displayPositionAt(float x, float y) const noexcept;
    [[nodiscard]] float boardWidth() const noexcept { return cellSize * static_cast<float>(BoardColumns); }
    [[nodiscard]] float boardHeight() const noexcept { return cellSize * static_cast<float>(BoardRows); }
};

} // namespace sts
