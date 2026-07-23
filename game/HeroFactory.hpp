#pragma once

#include "common/PieceColor.hpp"
#include "common/Position.hpp"
#include "common/GameTypes.hpp"
#include "game/Hero.hpp"

#include <memory>
#include <optional>

namespace sts {

class HeroFactory {
public:
    [[nodiscard]] static std::unique_ptr<Hero> create(HeroId id,
                                                      PieceColor color,
                                                      Position position,
                                                      int attributeValue,
                                                      std::optional<int> creatorPlayerId = std::nullopt);
};

} // namespace sts
