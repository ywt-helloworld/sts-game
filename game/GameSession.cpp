#include "game/GameSession.hpp"

namespace sts {

GameSession::GameSession() = default;

GameSession::GameSession(std::uint32_t boardSeed) : board_(boardSeed) {}

bool GameSession::markPlayerReady(int playerId) {
    if (phase_ != GamePhase::WaitingForPlayers || playerId < 0 || playerId > 1) {
        return false;
    }
    readyPlayers_[static_cast<std::size_t>(playerId)] = true;
    return true;
}

bool GameSession::start() {
    if (phase_ != GamePhase::WaitingForPlayers || !readyPlayers_[0] || !readyPlayers_[1]) {
        return false;
    }
    phase_ = GamePhase::Playing;
    return true;
}

void GameSession::markPlayerDisconnected(int playerId) noexcept {
    if (playerId < 0 || playerId > 1) {
        return;
    }
    readyPlayers_[static_cast<std::size_t>(playerId)] = false;
    if (phase_ == GamePhase::Playing || phase_ == GamePhase::ResolvingTurn) {
        phase_ = GamePhase::GameOver;
    }
}

EliminateResult GameSession::rejected(std::string message) const {
    return EliminateResult{false, std::move(message), currentPlayerId_, turnId_, board_.snapshot()};
}

EliminateResult GameSession::handleEliminateRequest(const EliminateRequest& request) {
    if (phase_ != GamePhase::Playing) {
        return rejected("the game is not accepting turns");
    }
    if (request.playerId != currentPlayerId_) {
        return rejected("it is not this player's turn");
    }
    if (request.turnId != turnId_) {
        return rejected("request turnId is stale or duplicated");
    }

    const ValidationResult validation = rule_.validate(board_, request.playerId, request.path);
    if (!validation.valid) {
        return rejected(validation.errorMessage);
    }

    phase_ = GamePhase::ResolvingTurn;
    board_.resolveElimination(request.playerId, request.path);
    currentPlayerId_ = 1 - currentPlayerId_;
    ++turnId_;
    phase_ = GamePhase::Playing;
    return EliminateResult{true, {}, currentPlayerId_, turnId_, board_.snapshot()};
}

} // namespace sts
