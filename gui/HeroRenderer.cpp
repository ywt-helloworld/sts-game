#include "gui/HeroRenderer.hpp"

#include "gui/HeroTextureCatalog.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <string>

namespace sts {
namespace {

sf::Color heroColor(PieceColor color) {
    switch (color) {
    case PieceColor::Red: return {218, 72, 72};
    case PieceColor::Yellow: return {235, 193, 64};
    case PieceColor::Green: return {70, 180, 105};
    case PieceColor::Blue: return {72, 128, 218};
    }
    return sf::Color::White;
}

void drawCenteredText(sf::RenderTarget& target,
                      const sf::Font* font,
                      const std::string& value,
                      unsigned characterSize,
                      sf::Vector2f center) {
    if (font == nullptr) {
        return;
    }
    sf::Text text(*font, sf::String::fromUtf8(value.begin(), value.end()), characterSize);
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({bounds.position.x + bounds.size.x / 2.0F,
                    bounds.position.y + bounds.size.y / 2.0F});
    text.setPosition(center);
    text.setFillColor(sf::Color::White);
    target.draw(text);
}

} // namespace

HeroRenderer::HeroRenderer(const ResourceManager& resources) noexcept
    : resources_(resources), statusRenderer_(resources) {}

void HeroRenderer::draw(sf::RenderTarget& target,
                        const PieceSnapshot& hero,
                        const sf::FloatRect& cellRect,
                        bool selected,
                        const sf::Font* font) const {
    const sf::Vector2f cellCenter{cellRect.position.x + cellRect.size.x / 2.0F,
                                  cellRect.position.y + cellRect.size.y / 2.0F};

    if (selected) {
        sf::RectangleShape selectionFrame({cellRect.size.x - 4.0F, cellRect.size.y - 4.0F});
        selectionFrame.setPosition({cellRect.position.x + 2.0F, cellRect.position.y + 2.0F});
        selectionFrame.setFillColor(sf::Color::Transparent);
        selectionFrame.setOutlineColor(sf::Color{246, 221, 150, 210});
        selectionFrame.setOutlineThickness(-2.0F);
        target.draw(selectionFrame);
    }

    const float shadowRadius = cellRect.size.x * 0.33F;
    sf::CircleShape shadow(shadowRadius);
    shadow.setScale({1.0F, 0.28F});
    shadow.setPosition({cellCenter.x - shadowRadius,
                        cellRect.position.y + cellRect.size.y * 0.72F});
    shadow.setFillColor(selected ? sf::Color{10, 10, 14, 145}
                                 : sf::Color{10, 10, 14, 105});
    target.draw(shadow);

    if (const sf::Texture* texture = resources_.heroTextureFor(hero.color); texture != nullptr) {
        const sf::Vector2u textureSize = texture->getSize();
        const HeroVisualLayout layout = fitHeroTextureInsideCell(textureSize.x,
                                                                 textureSize.y,
                                                                 cellRect.position.x,
                                                                 cellRect.position.y,
                                                                 cellRect.size.x,
                                                                 cellRect.size.y,
                                                                 selected);
        sf::Sprite sprite(*texture);
        sprite.setScale({layout.uniformScale, layout.uniformScale});
        sprite.setPosition({layout.left, layout.top});
        target.draw(sprite);
    } else {
        const float fallbackSize = cellRect.size.x * 0.72F * heroSelectionScale(selected);
        sf::RectangleShape fallback({fallbackSize, fallbackSize});
        fallback.setPosition({cellCenter.x - fallbackSize / 2.0F,
                              cellCenter.y - fallbackSize / 2.0F});
        fallback.setFillColor(sf::Color{24, 27, 34, 210});
        fallback.setOutlineColor(heroColor(hero.color));
        fallback.setOutlineThickness(-3.0F);
        target.draw(fallback);
    }

    const float badgeRadius = std::max(8.0F, cellRect.size.x * 0.135F);
    const sf::Vector2f badgePosition{cellRect.position.x + cellRect.size.x - badgeRadius * 2.0F - 4.0F,
                                     cellRect.position.y + 4.0F};
    sf::CircleShape badge(badgeRadius);
    badge.setPosition(badgePosition);
    badge.setFillColor(sf::Color{18, 20, 26, 225});
    badge.setOutlineColor(heroColor(hero.color));
    badge.setOutlineThickness(2.0F);
    target.draw(badge);
    drawCenteredText(target,
                     font,
                     std::to_string(hero.attributeValue),
                     std::max(11U, static_cast<unsigned>(cellRect.size.x * 0.16F)),
                     {badgePosition.x + badgeRadius, badgePosition.y + badgeRadius});

    const sf::Vector2f hpSize{cellRect.size.x * 0.84F, std::max(18.0F, cellRect.size.y * 0.18F)};
    const sf::Vector2f hpPosition{cellRect.position.x + (cellRect.size.x - hpSize.x) / 2.0F,
                                  cellRect.position.y + cellRect.size.y - hpSize.y - 3.0F};
    sf::RectangleShape hpPanel(hpSize);
    hpPanel.setPosition(hpPosition);
    hpPanel.setFillColor(sf::Color{18, 20, 26, 225});
    hpPanel.setOutlineColor(heroColor(hero.color));
    hpPanel.setOutlineThickness(1.0F);
    target.draw(hpPanel);
    drawCenteredText(target,
                     font,
                     std::to_string(hero.currentHp) + " / " + std::to_string(hero.maxHp),
                     std::max(10U, static_cast<unsigned>(cellRect.size.x * 0.125F)),
                     {hpPosition.x + hpSize.x / 2.0F, hpPosition.y + hpSize.y / 2.0F});

    const float statusBottom = hpPosition.y - 2.0F;
    statusRenderer_.drawHeroStatuses(target, hero, cellRect, statusBottom, font);

    const StatusStripMetrics statusMetrics =
        statusStripMetrics(heroStatusDisplayItems(hero).size(), cellRect.size.x);
    const float resourceBottom =
        statusBottom - (statusMetrics.height > 0.0F
                            ? statusMetrics.height +
                                  std::max(3.0F, cellRect.size.y * 0.025F)
                            : 0.0F);
    statusRenderer_.drawHeroResource(target, hero, cellRect, resourceBottom, font);
}

} // namespace sts
