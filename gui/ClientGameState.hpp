#pragma once

#include "common/Protocol.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace sts {

enum class ClientConnectionPhase { Connecting, Connected, WaitingForOpponent, Playing, GameOver, Disconnected };

class ClientGameState {
public:
    [[nodiscard]] bool canSelect() const noexcept;
    [[nodiscard]] bool canSubmit() const noexcept;
    [[nodiscard]] const PieceSnapshot* pieceAtLogical(Position logical) const noexcept;

    void apply(const GameStartedMessage& message);
    void apply(const EliminateResult& result);
    void setPlayerId(int playerId);
    void setStatus(std::string status);
    void setError(std::string error);

    std::optional<BoardSnapshot> board;
    GamePhase gamePhase{GamePhase::WaitingForPlayers};
    std::array<TowerSnapshot, 2> towers{};
    std::optional<int> winnerPlayerId;
    std::vector<CombatEvent> combatEvents;
    int playerId{-1};
    int currentPlayerId{-1};
    int turnId{};
    bool openingTurnPending{true};
    bool requestPending{};
    ClientConnectionPhase phase{ClientConnectionPhase::Connecting};
    std::string status{"正在连接服务器"};
    std::string lastError;
};

} // namespace sts
