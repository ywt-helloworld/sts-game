#pragma once

#include "common/GameTypes.hpp"
#include "game/CombatRandom.hpp"

#include <array>
#include <optional>
#include <vector>

namespace sts {

class BoxBoard;
class Tower;

struct CombatResolution {
    std::vector<CombatEvent> events;
    bool gameFinished{};
    std::optional<int> winnerPlayerId;
};

class CombatResolver {
public:
    CombatResolver(BoxBoard& board, std::array<Tower, 2>& towers) noexcept;
    CombatResolver(BoxBoard& board, std::array<Tower, 2>& towers, CombatRandom& random) noexcept;

    [[nodiscard]] std::vector<HeroId> collectAttackers(int actingPlayerId) const;
    [[nodiscard]] CombatResolution resolve(int actingPlayerId);

private:
    void processTurnEndStatuses(std::vector<CombatEvent>& events, int actingPlayerId);

    BoxBoard& board_;
    std::array<Tower, 2>& towers_;
    CombatRandom ownedRandom_{0x535453U};
    CombatRandom* random_{};
};

} // namespace sts
