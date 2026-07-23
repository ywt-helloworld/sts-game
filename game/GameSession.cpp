#include "game/GameSession.hpp"

#include "game/CombatResolver.hpp"

#include <stdexcept>
#include <utility>

namespace sts {

GameSession::GameSession() = default;

GameSession::GameSession(std::uint32_t boardSeed)
    : board_(boardSeed), combatRandom_(boardSeed ^ 0x9e3779b9U) {}

GameSession::GameSession(std::uint32_t boardSeed, std::uint32_t combatSeed)
    : board_(boardSeed), combatRandom_(combatSeed) {}

GameSession::GameSession(std::uint32_t boardSeed, std::uint32_t combatSeed, int firstPlayerId)
    : currentPlayerId_(firstPlayerId), board_(boardSeed), combatRandom_(combatSeed) {
    if (firstPlayerId != 0 && firstPlayerId != 1) {
        throw std::invalid_argument("firstPlayerId must be 0 or 1");
    }
}

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
    openingTurnPending_ = true;
    phase_ = GamePhase::Elimination;
    return true;
}

void GameSession::markPlayerDisconnected(int playerId) noexcept {
    if (playerId < 0 || playerId > 1) {
        return;
    }
    readyPlayers_[static_cast<std::size_t>(playerId)] = false;
    if (phase_ != GamePhase::WaitingForPlayers) {
        phase_ = GamePhase::Finished;
    }
}

GameSnapshot GameSession::snapshot() const {
    return {phase_,
            currentPlayerId_,
            turnId_,
            winnerPlayerId_,
            {towers_[0].snapshot(), towers_[1].snapshot()},
            board_.snapshot(),
            openingTurnPending_};
}

EliminateResult GameSession::rejected(std::string message) const {
    return {false, std::move(message), snapshot(), {}};
}

EliminateResult GameSession::handleEliminateRequest(const EliminateRequest& request) {
    if (phase_ != GamePhase::Elimination) {
        return rejected("the game is not accepting elimination requests in the current phase");
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

    static_cast<void>(board_.resolveElimination(request.playerId, request.path));

    if (openingTurnPending_) {
        openingTurnPending_ = false;
        const int firstPlayerId = currentPlayerId_;
        currentPlayerId_ = 1 - currentPlayerId_;
        ++turnId_;
        phase_ = GamePhase::Elimination;

        CombatEvent openingCompleted;
        openingCompleted.type = CombatEventType::OpeningTurnCompleted;
        openingCompleted.actingPlayerId = firstPlayerId;
        openingCompleted.targetPlayerId = currentPlayerId_;
        openingCompleted.amount = turnId_;

        CombatEvent turnChanged;
        turnChanged.type = CombatEventType::TurnChanged;
        turnChanged.actingPlayerId = currentPlayerId_;
        turnChanged.targetPlayerId = firstPlayerId;
        return {true, {}, snapshot(), {openingCompleted, turnChanged}};
    }

    phase_ = GamePhase::Combat;
    CombatResolver resolver(board_, towers_, combatRandom_);
    CombatResolution combat = resolver.resolve(request.playerId);

    if (combat.gameFinished) {
        phase_ = GamePhase::Finished;
        winnerPlayerId_ = combat.winnerPlayerId;
    } else {
        currentPlayerId_ = 1 - currentPlayerId_;
        ++turnId_;
        phase_ = GamePhase::Elimination;
        combat.events.push_back({CombatEventType::TurnChanged, std::nullopt, std::nullopt,
                                 currentPlayerId_, 1 - currentPlayerId_, 0, 0, std::nullopt});
    }
    return {true, {}, snapshot(), std::move(combat.events)};
}

} // namespace sts
