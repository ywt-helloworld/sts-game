#pragma once

#include "common/GameTypes.hpp"

namespace sts {

class Tower {
public:
    static constexpr int MaxHp = 1000;

    explicit Tower(int playerId);

    [[nodiscard]] int playerId() const noexcept { return playerId_; }
    [[nodiscard]] int currentHp() const noexcept { return currentHp_; }
    [[nodiscard]] int maxHp() const noexcept { return MaxHp; }
    [[nodiscard]] bool isDestroyed() const noexcept { return currentHp_ == 0; }
    [[nodiscard]] int takeDamage(int damage) noexcept;
    [[nodiscard]] TowerSnapshot snapshot() const noexcept { return {playerId_, currentHp_, MaxHp}; }

private:
    int playerId_{};
    int currentHp_{MaxHp};
};

} // namespace sts
