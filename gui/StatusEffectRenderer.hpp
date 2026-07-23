#pragma once

#include "gui/ResourceManager.hpp"
#include "gui/StatusEffectDisplay.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <vector>

namespace sts {

class StatusEffectRenderer {
public:
    explicit StatusEffectRenderer(const ResourceManager& resources) noexcept;

    void drawHeroStatuses(sf::RenderTarget& target,
                          const PieceSnapshot& hero,
                          const sf::FloatRect& cellRect,
                          float bottom,
                          const sf::Font* font) const;

    void drawTowerStatuses(sf::RenderTarget& target,
                           const TowerSnapshot& tower,
                           const sf::FloatRect& bounds,
                           const sf::Font* font) const;

private:
    void drawItems(sf::RenderTarget& target,
                   const std::vector<StatusDisplayItem>& items,
                   const sf::FloatRect& bounds,
                   float referenceWidth,
                   const sf::Font* font) const;

    const ResourceManager& resources_;
};

} // namespace sts
