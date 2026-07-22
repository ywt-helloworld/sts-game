#pragma once

#include "common/PieceColor.hpp"
#include "common/Position.hpp"
#include "game/Hero.hpp"

#include <memory>

namespace sts {

class HeroFactory {
public:
    [[nodiscard]] static std::unique_ptr<Hero> create(PieceColor color, Position position, int attribute);
};

} // namespace sts
