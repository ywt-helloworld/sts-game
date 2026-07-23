#pragma once

#include "common/PieceColor.hpp"
#include "gui/StatusEffectDisplay.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <memory>
#include <vector>

namespace sts {

struct BoxTextureSet {
    const sf::Texture* normal{};
    const sf::Texture* selected{};
};

class ResourceManager {
public:
    ResourceManager();
    [[nodiscard]] const sf::Font* font() const noexcept;
    [[nodiscard]] const sf::Texture* background() const noexcept;
    [[nodiscard]] const BoxTextureSet& texturesFor(PieceColor color) const noexcept;
    [[nodiscard]] const sf::Texture* heroTextureFor(PieceColor color) const noexcept;
    [[nodiscard]] const sf::Texture* statusTextureFor(StatusIconKind kind) const noexcept;

private:
    [[nodiscard]] const sf::Texture* loadBoxTexture(PieceColor color, bool selected);
    [[nodiscard]] const sf::Texture* loadHeroTexture(PieceColor color);
    [[nodiscard]] const sf::Texture* loadStatusTexture(StatusIconKind kind);
    void loadBackgroundTexture();

    sf::Font font_;
    bool fontLoaded_{};
    std::array<BoxTextureSet, 4> boxTextures_{};
    std::array<const sf::Texture*, 4> heroTextures_{};
    std::array<const sf::Texture*, 3> statusTextures_{};
    std::unique_ptr<sf::Texture> backgroundTexture_;
    std::vector<std::unique_ptr<sf::Texture>> ownedTextures_;
};

} // namespace sts
