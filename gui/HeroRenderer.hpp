#pragma once

#include "common/Protocol.hpp"
#include "gui/ResourceManager.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace sts {

class HeroRenderer {
public:
    explicit HeroRenderer(const ResourceManager& resources) noexcept;

    void draw(sf::RenderTarget& target,
              const PieceSnapshot& hero,
              const sf::FloatRect& cellRect,
              bool selected,
              const sf::Font* font) const;

private:
    const ResourceManager& resources_;
};

} // namespace sts
