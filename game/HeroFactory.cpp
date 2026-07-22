#include "game/HeroFactory.hpp"

#include "game/Hero.hpp"

namespace sts {

std::unique_ptr<Hero> HeroFactory::create(PieceColor color, Position position, int attribute) {
    switch (color) {
    case PieceColor::Red: return std::make_unique<RedHero>(position, attribute);
    case PieceColor::Yellow: return std::make_unique<YellowHero>(position, attribute);
    case PieceColor::Green: return std::make_unique<GreenHero>(position, attribute);
    case PieceColor::Blue: return std::make_unique<BlueHero>(position, attribute);
    }
    return nullptr;
}

} // namespace sts
