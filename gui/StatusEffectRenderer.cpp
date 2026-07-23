#include "gui/StatusEffectRenderer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <string>

namespace sts {

StatusEffectRenderer::StatusEffectRenderer(const ResourceManager& resources) noexcept
    : resources_(resources) {}

void StatusEffectRenderer::drawHeroStatuses(sf::RenderTarget& target,
                                            const PieceSnapshot& hero,
                                            const sf::FloatRect& cellRect,
                                            float bottom,
                                            const sf::Font* font) const {
    const std::vector<StatusDisplayItem> items = heroStatusDisplayItems(hero);
    const StatusStripMetrics metrics = statusStripMetrics(items.size(), cellRect.size.x);
    if (items.empty()) {
        return;
    }
    drawItems(target,
              items,
              {{cellRect.position.x + 2.0F, bottom - metrics.height},
               {cellRect.size.x - 4.0F, metrics.height}},
              cellRect.size.x,
              font);
}

void StatusEffectRenderer::drawTowerStatuses(sf::RenderTarget& target,
                                             const TowerSnapshot& tower,
                                             const sf::FloatRect& bounds,
                                             const sf::Font* font) const {
    drawItems(target,
              towerStatusDisplayItems(tower),
              bounds,
              120.0F,
              font);
}

void StatusEffectRenderer::drawItems(sf::RenderTarget& target,
                                     const std::vector<StatusDisplayItem>& items,
                                     const sf::FloatRect& bounds,
                                     float referenceWidth,
                                     const sf::Font* font) const {
    const StatusStripMetrics metrics = statusStripMetrics(items.size(), referenceWidth);
    if (items.empty() || metrics.totalWidth <= 0.0F) {
        return;
    }

    float left = bounds.position.x + std::max(0.0F, (bounds.size.x - metrics.totalWidth) / 2.0F);
    const float top = bounds.position.y + std::max(0.0F, (bounds.size.y - metrics.height) / 2.0F);
    for (const StatusDisplayItem& item : items) {
        sf::RectangleShape panel({metrics.panelSize, metrics.panelSize});
        panel.setPosition({left, top});
        panel.setFillColor(sf::Color{8, 14, 22, 166});
        panel.setOutlineColor(sf::Color{210, 224, 236, 75});
        panel.setOutlineThickness(1.0F);
        target.draw(panel);

        const sf::Texture* texture = resources_.statusTextureFor(item.kind);
        if (texture != nullptr) {
            const sf::Vector2u textureSize = texture->getSize();
            const float scale = std::min(metrics.iconSize / static_cast<float>(textureSize.x),
                                         metrics.iconSize / static_cast<float>(textureSize.y));
            const sf::Vector2f renderedSize{
                static_cast<float>(textureSize.x) * scale,
                static_cast<float>(textureSize.y) * scale,
            };
            sf::Sprite sprite(*texture);
            sprite.setScale({scale, scale});
            sprite.setPosition({
                left + (metrics.panelSize - renderedSize.x) / 2.0F,
                top + (metrics.panelSize - renderedSize.y) / 2.0F,
            });
            target.draw(sprite);
        }

        if (font != nullptr) {
            const unsigned characterSize =
                std::max(11U, static_cast<unsigned>(metrics.iconSize * 0.64F));
            const std::string value = std::to_string(item.value);
            sf::Text text(*font, sf::String::fromUtf8(value.begin(), value.end()), characterSize);
            const sf::FloatRect textBounds = text.getLocalBounds();
            text.setOrigin({
                textBounds.position.x + textBounds.size.x,
                textBounds.position.y + textBounds.size.y,
            });
            text.setPosition({
                left + metrics.panelSize - 1.0F,
                top + metrics.panelSize - 1.0F,
            });
            text.setFillColor(sf::Color::White);
            text.setOutlineColor(sf::Color{4, 5, 8, 245});
            text.setOutlineThickness(std::max(1.0F, metrics.iconSize * 0.075F));
            target.draw(text);
        }

        left += metrics.panelSize + metrics.gap;
    }
}

} // namespace sts
