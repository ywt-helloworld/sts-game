#include "game/Tower.hpp"

#include "game/CombatMath.hpp"

#include <algorithm>
#include <stdexcept>

namespace sts {

Tower::Tower(int playerId) : playerId_(playerId) {
    if (playerId != 0 && playerId != 1) {
        throw std::invalid_argument("tower playerId must be 0 or 1");
    }
}

int Tower::takeDamage(int damage) noexcept {
    const int applied = std::min(currentHp_, std::max(0, damage));
    currentHp_ -= applied;
    return applied;
}

void Tower::addVulnerableLayers(int layers) {
    if (layers > 0 && !isDestroyed()) {
        vulnerableLayers_ = checkedAdd(vulnerableLayers_, layers);
    }
}

void Tower::addWeakLayers(int layers) {
    if (layers > 0 && !isDestroyed()) {
        weakLayers_ = checkedAdd(weakLayers_, layers);
    }
}

void Tower::processTurnEndStatuses() noexcept {
    vulnerableLayers_ = std::max(0, vulnerableLayers_ - 1);
    weakLayers_ = std::max(0, weakLayers_ - 1);
}

} // namespace sts
