#include "game/Tower.hpp"

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

} // namespace sts
