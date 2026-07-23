#include "game/HeroFactory.hpp"

#include "game/Hero.hpp"

namespace sts {

std::unique_ptr<Hero> HeroFactory::create(HeroId id,
                                          PieceColor color,
                                          Position position,
                                          int attributeValue,
                                          std::optional<int> creatorPlayerId) {
    switch (color) {
    case PieceColor::Red: return std::make_unique<IronFighter>(id, position, attributeValue, creatorPlayerId);
    case PieceColor::Yellow: return std::make_unique<Regent>(id, position, attributeValue, creatorPlayerId);
    case PieceColor::Green: return std::make_unique<SilentHunter>(id, position, attributeValue, creatorPlayerId);
    case PieceColor::Blue: return std::make_unique<ChickenPot>(id, position, attributeValue, creatorPlayerId);
    }
    return nullptr;
}

} // namespace sts
