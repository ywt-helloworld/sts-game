#pragma once

#include "common/Position.hpp"
#include "gui/ClientGameState.hpp"

#include <vector>

namespace sts {

class InputController {
public:
    [[nodiscard]] bool trySelect(Position displayPosition, const ClientGameState& state);
    [[nodiscard]] bool undoLast() noexcept;
    void clear() noexcept { path_.clear(); }
    void applyServerResult(const EliminateResult& result) noexcept;
    [[nodiscard]] bool isSelected(Position displayPosition) const noexcept;
    [[nodiscard]] bool canConfirm(const ClientGameState& state) const noexcept;
    [[nodiscard]] EliminateRequest makeRequest(const ClientGameState& state) const;
    [[nodiscard]] const std::vector<Position>& path() const noexcept { return path_; }

private:
    [[nodiscard]] static bool areEightNeighbors(Position first, Position second) noexcept;

    std::vector<Position> path_;
};

} // namespace sts
