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
    [[nodiscard]] int vulnerableLayers() const noexcept { return vulnerableLayers_; }
    [[nodiscard]] int weakLayers() const noexcept { return weakLayers_; }
    [[nodiscard]] bool isVulnerable() const noexcept { return vulnerableLayers_ > 0; }
    [[nodiscard]] bool isWeak() const noexcept { return weakLayers_ > 0; }
    [[nodiscard]] int takeDamage(int damage) noexcept;
    void addVulnerableLayers(int layers);
    void addWeakLayers(int layers);
    void processTurnEndStatuses() noexcept;
    [[nodiscard]] TowerSnapshot snapshot() const noexcept {
        return {playerId_, currentHp_, MaxHp, vulnerableLayers_, weakLayers_, isDestroyed()};
    }

private:
    int playerId_{};
    int currentHp_{MaxHp};
    int vulnerableLayers_{};
    int weakLayers_{};
};

} // namespace sts
