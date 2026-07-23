#pragma once

#include "common/Protocol.hpp"
#include "game/BoxBoard.hpp"
#include "game/CombatRandom.hpp"
#include "game/EliminationRule.hpp"
#include "game/Tower.hpp"

#include <array>
#include <optional>

namespace sts {

class GameSession {
public:
    GameSession();
    explicit GameSession(std::uint32_t boardSeed);
    GameSession(std::uint32_t boardSeed, std::uint32_t combatSeed);
    GameSession(std::uint32_t boardSeed, std::uint32_t combatSeed, int firstPlayerId);

    [[nodiscard]] GamePhase phase() const noexcept { return phase_; }
    [[nodiscard]] int currentPlayerId() const noexcept { return currentPlayerId_; }
    [[nodiscard]] int turnId() const noexcept { return turnId_; }
    [[nodiscard]] bool openingTurnPending() const noexcept { return openingTurnPending_; }
    [[nodiscard]] std::optional<int> winnerPlayerId() const noexcept { return winnerPlayerId_; }
    [[nodiscard]] const BoxBoard& board() const noexcept { return board_; }
    [[nodiscard]] BoxBoard& boardForSetup() noexcept { return board_; }
    [[nodiscard]] const std::array<Tower, 2>& towers() const noexcept { return towers_; }
    [[nodiscard]] std::array<Tower, 2>& towersForSetup() noexcept { return towers_; }
    [[nodiscard]] GameSnapshot snapshot() const;

    [[nodiscard]] bool markPlayerReady(int playerId);
    [[nodiscard]] bool start();
    void markPlayerDisconnected(int playerId) noexcept;
    void endGame() noexcept { phase_ = GamePhase::Finished; }
    [[nodiscard]] EliminateResult handleEliminateRequest(const EliminateRequest& request);

private:
    [[nodiscard]] EliminateResult rejected(std::string message) const;

    GamePhase phase_{GamePhase::WaitingForPlayers};
    int currentPlayerId_{};
    int turnId_{1};
    std::optional<int> winnerPlayerId_;
    std::array<bool, 2> readyPlayers_{};
    bool openingTurnPending_{true};
    std::array<Tower, 2> towers_{Tower{0}, Tower{1}};
    BoxBoard board_;
    CombatRandom combatRandom_{0x535453U};
    EliminationRule rule_;
};

} // namespace sts
