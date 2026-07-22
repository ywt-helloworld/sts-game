#pragma once

#include "common/BoardDimensions.hpp"
#include "common/Position.hpp"

namespace sts {

class PlayerViewTransform {
public:
    [[nodiscard]] static constexpr Position logicalToDisplay(int playerId, Position logical) noexcept {
        return playerId == 1
            ? Position{BoardRows - 1 - logical.row, BoardColumns - 1 - logical.column}
            : logical;
    }

    [[nodiscard]] static constexpr Position displayToLogical(int playerId, Position display) noexcept {
        return logicalToDisplay(playerId, display);
    }

    [[nodiscard]] static constexpr bool isOwnDisplayArea(Position display) noexcept {
        return display.row >= PlayerAreaRows && display.row < BoardRows &&
               display.column >= 0 && display.column < BoardColumns;
    }
};

} // namespace sts
