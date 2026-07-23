#include "gui/ClientGameState.hpp"

#include "common/BoardDimensions.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace sts {

bool ClientGameState::canSelect() const noexcept {
    return phase == ClientConnectionPhase::Playing && board.has_value() && playerId >= 0 &&
           gamePhase == GamePhase::Elimination && currentPlayerId == playerId && !requestPending;
}

bool ClientGameState::canSubmit() const noexcept {
    return canSelect();
}

const PieceSnapshot* ClientGameState::pieceAtLogical(Position logical) const noexcept {
    if (!board.has_value() || logical.row < 0 || logical.row >= BoardRows ||
        logical.column < 0 || logical.column >= BoardColumns) {
        return nullptr;
    }
    return &(*board)[logical.row][logical.column];
}

void ClientGameState::apply(const GameStartedMessage& message) {
    board = message.game.board;
    gamePhase = message.game.phase;
    currentPlayerId = message.game.currentPlayerId;
    turnId = message.game.turnId;
    openingTurnPending = message.game.openingTurnPending;
    towers = message.game.towers;
    winnerPlayerId = message.game.winnerPlayerId;
    combatEvents.clear();
    requestPending = false;
    phase = ClientConnectionPhase::Playing;
    if (openingTurnPending) {
        status = currentPlayerId == playerId
                     ? "开局布阵回合：本回合只生成英雄，不会发动攻击"
                     : "等待先手玩家完成开局布阵";
    } else {
        status = currentPlayerId == playerId ? "消除阶段：轮到你" : "等待对方消除";
    }
    lastError.clear();
}

void ClientGameState::apply(const EliminateResult& result) {
    board = result.game.board;
    gamePhase = result.game.phase;
    currentPlayerId = result.game.currentPlayerId;
    turnId = result.game.turnId;
    openingTurnPending = result.game.openingTurnPending;
    towers = result.game.towers;
    winnerPlayerId = result.game.winnerPlayerId;
    combatEvents = result.combatEvents;
    requestPending = false;
    if (result.success) {
        const bool openingCompleted = std::ranges::any_of(result.combatEvents, [](const CombatEvent& event) {
            return event.type == CombatEventType::OpeningTurnCompleted;
        });
        if (gamePhase == GamePhase::Finished) {
            phase = ClientConnectionPhase::GameOver;
            status = winnerPlayerId == playerId ? "游戏结束：你获胜" : "游戏结束：你失败";
        } else if (openingCompleted) {
            status = currentPlayerId == playerId
                         ? "开局布阵完成，轮到你进行正常消除"
                         : "开局布阵完成，等待对方操作";
        } else {
            status = currentPlayerId == playerId ? "战斗结算完成，轮到你消除"
                                                 : "战斗结算完成，等待对方消除";
        }
        lastError.clear();
    } else {
        status = "操作失败";
        lastError = result.errorMessage;
    }
}

void ClientGameState::setPlayerId(int value) {
    playerId = value;
    if (phase == ClientConnectionPhase::Connecting) {
        phase = ClientConnectionPhase::Connected;
    }
}

void ClientGameState::setStatus(std::string value) {
    status = std::move(value);
}

void ClientGameState::setError(std::string value) {
    lastError = std::move(value);
}

} // namespace sts
