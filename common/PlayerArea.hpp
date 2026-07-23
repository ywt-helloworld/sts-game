#pragma once

#include "common/BoardDimensions.hpp"
#include "common/Position.hpp"

#include <optional>

namespace sts {

[[nodiscard]] inline std::optional<int> playerIdForPosition(Position position) noexcept {
    if (position.row < 0 || position.row >= BoardRows ||
        position.column < 0 || position.column >= BoardColumns) {
        return std::nullopt;
    }
    return position.row < PlayerAreaRows ? std::optional<int>{1} : std::optional<int>{0};
}

} // namespace sts
