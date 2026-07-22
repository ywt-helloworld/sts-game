#pragma once

#include "common/Protocol.hpp"
#include "game/BoxBoard.hpp"
#include "game/EliminationRule.hpp"

#include <array>

namespace sts {

enum class GamePhase { WaitingForPlayers, Playing, ResolvingTurn, GameOver };

class GameSession {
public:
    GameSession();
    explicit GameSession(std::uint32_t boardSeed);

    [[nodiscard]] GamePhase phase() const noexcept { return phase_; }
    [[nodiscard]] int currentPlayerId() const noexcept { return currentPlayerId_; }
    [[nodiscard]] int turnId() const noexcept { return turnId_; }
    [[nodiscard]] const BoxBoard& board() const noexcept { return board_; }
    [[nodiscard]] BoxBoard& boardForSetup() noexcept { return board_; }

    // The network room marks active slots ready, then explicitly starts only
    // after it has confirmed both TCP sessions are still connected.
    [[nodiscard]] bool markPlayerReady(int playerId);
    [[nodiscard]] bool start();
    void markPlayerDisconnected(int playerId) noexcept;
    void endGame() noexcept { phase_ = GamePhase::GameOver; }
    [[nodiscard]] EliminateResult handleEliminateRequest(const EliminateRequest& request);

private:
    [[nodiscard]] EliminateResult rejected(std::string message) const;

    GamePhase phase_{GamePhase::WaitingForPlayers};
    int currentPlayerId_{};
    int turnId_{1};
    std::array<bool, 2> readyPlayers_{};
    BoxBoard board_;
    EliminationRule rule_;
};

} // namespace sts
