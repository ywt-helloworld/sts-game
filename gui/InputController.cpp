#include "gui/InputController.hpp"

#include "common/PlayerViewTransform.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sts {

bool InputController::trySelect(Position displayPosition, const ClientGameState& state) {
    if (!state.canSelect() || !PlayerViewTransform::isOwnDisplayArea(displayPosition)) {
        return false;
    }
    if (!path_.empty() && path_.back() == displayPosition) {
        return undoLast();
    }
    if (std::find(path_.begin(), path_.end(), displayPosition) != path_.end()) {
        return false;
    }
    const Position logical = PlayerViewTransform::displayToLogical(state.playerId, displayPosition);
    const PieceSnapshot* piece = state.pieceAtLogical(logical);
    if (piece == nullptr) {
        return false;
    }
    if (!path_.empty()) {
        if (!areEightNeighbors(path_.back(), displayPosition)) {
            return false;
        }
        const Position previousLogical = PlayerViewTransform::displayToLogical(state.playerId, path_.back());
        const PieceSnapshot* previous = state.pieceAtLogical(previousLogical);
        if (previous == nullptr || previous->color != piece->color) {
            return false;
        }
    }
    path_.push_back(displayPosition);
    return true;
}

bool InputController::undoLast() noexcept {
    if (path_.empty()) {
        return false;
    }
    path_.pop_back();
    return true;
}

void InputController::applyServerResult(const EliminateResult& result) noexcept {
    if (result.success) {
        clear();
    }
}

bool InputController::isSelected(Position displayPosition) const noexcept {
    return std::find(path_.begin(), path_.end(), displayPosition) != path_.end();
}

bool InputController::canConfirm(const ClientGameState& state) const noexcept {
    return state.canSubmit() && path_.size() >= 3U;
}

EliminateRequest InputController::makeRequest(const ClientGameState& state) const {
    if (!canConfirm(state)) {
        throw std::logic_error("cannot submit the current path");
    }
    EliminateRequest request;
    request.playerId = state.playerId;
    request.turnId = state.turnId;
    request.path.reserve(path_.size());
    for (const Position display : path_) {
        request.path.push_back(PlayerViewTransform::displayToLogical(state.playerId, display));
    }
    return request;
}

bool InputController::areEightNeighbors(Position first, Position second) noexcept {
    return first != second && std::abs(first.row - second.row) <= 1 &&
           std::abs(first.column - second.column) <= 1;
}

} // namespace sts
