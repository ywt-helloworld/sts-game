#include "gui/ResourceManager.hpp"

#include "gui/BoxTextureCatalog.hpp"
#include "gui/HeroTextureCatalog.hpp"

#include <SFML/Graphics/Image.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace sts {
namespace {

std::optional<sf::IntRect> visiblePixelBounds(const sf::Image& image) {
    constexpr std::uint8_t alphaThreshold = 8U;
    const sf::Vector2u size = image.getSize();
    unsigned left = size.x;
    unsigned top = size.y;
    unsigned right{};
    unsigned bottom{};
    bool found{};
    for (unsigned y = 0; y < size.y; ++y) {
        for (unsigned x = 0; x < size.x; ++x) {
            if (image.getPixel({x, y}).a <= alphaThreshold) {
                continue;
            }
            found = true;
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x);
            bottom = std::max(bottom, y);
        }
    }
    if (!found) {
        return std::nullopt;
    }
    return sf::IntRect{{static_cast<int>(left), static_cast<int>(top)},
                       {static_cast<int>(right - left + 1U), static_cast<int>(bottom - top + 1U)}};
}

} // namespace

ResourceManager::ResourceManager() {
#ifdef _WIN32
    constexpr std::array candidates{"C:/Windows/Fonts/msyh.ttc",
                                    "C:/Windows/Fonts/simsun.ttc",
                                    "C:/Windows/Fonts/segoeui.ttf",
                                    "C:/Windows/Fonts/arial.ttf"};
#else
    constexpr std::array candidates{"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
                                    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                    "/usr/share/fonts/truetype/freefont/FreeSans.ttf"};
#endif
    for (const char* path : candidates) {
        if (font_.openFromFile(path)) {
            fontLoaded_ = true;
            break;
        }
    }

    for (int color = static_cast<int>(PieceColor::Red); color <= static_cast<int>(PieceColor::Blue); ++color) {
        const auto pieceColor = static_cast<PieceColor>(color);
        auto& textureSet = boxTextures_[static_cast<std::size_t>(color)];
        textureSet.normal = loadBoxTexture(pieceColor, false);
        textureSet.selected = loadBoxTexture(pieceColor, true);
        heroTextures_[static_cast<std::size_t>(color)] = loadHeroTexture(pieceColor);
    }
    for (int icon = static_cast<int>(HudIconKind::Vulnerable);
         icon <= static_cast<int>(HudIconKind::LightningChargeOrb);
         ++icon) {
        const auto kind = static_cast<HudIconKind>(icon);
        hudTextures_[static_cast<std::size_t>(icon)] = loadHudTexture(kind);
    }
    loadBackgroundTexture();
}

const sf::Font* ResourceManager::font() const noexcept {
    return fontLoaded_ ? &font_ : nullptr;
}

const sf::Texture* ResourceManager::background() const noexcept {
    return backgroundTexture_.get();
}

const BoxTextureSet& ResourceManager::texturesFor(PieceColor color) const noexcept {
    const auto index = static_cast<std::size_t>(color);
    return boxTextures_[index < boxTextures_.size() ? index : 0U];
}

const sf::Texture* ResourceManager::heroTextureFor(PieceColor color) const noexcept {
    const auto index = static_cast<std::size_t>(color);
    return heroTextures_[index < heroTextures_.size() ? index : 0U];
}

const sf::Texture* ResourceManager::hudTextureFor(HudIconKind kind) const noexcept {
    const auto index = static_cast<std::size_t>(kind);
    return hudTextures_[index < hudTextures_.size() ? index : 0U];
}

const sf::Texture* ResourceManager::loadBoxTexture(PieceColor color, bool selected) {
    const std::filesystem::path workingDirectory = std::filesystem::current_path();
    const std::vector<std::filesystem::path> searchDirectories{
        workingDirectory / "assets" / "tiles",
        workingDirectory.parent_path() / "assets" / "tiles",
    };
    const BoxTextureState state = boxTextureState(selected);
    const auto path = findBoxTextureFile(color, state, searchDirectories);
    if (!path.has_value()) {
        std::cerr << "Box texture not found: " << boxTextureFilename(color, state) << " (searched";
        for (const auto& directory : searchDirectories) {
            std::cerr << " " << (directory / boxTextureFilename(color, state)).string();
        }
        std::cerr << ")\n";
        return nullptr;
    }

    sf::Image image;
    if (!image.loadFromFile(*path)) {
        std::cerr << "Failed to load Box texture: " << path->string() << '\n';
        return nullptr;
    }
    const auto visibleBounds = visiblePixelBounds(image);
    if (!visibleBounds.has_value()) {
        std::cerr << "Box texture contains no visible pixels: " << path->string() << '\n';
        return nullptr;
    }

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromImage(image, false, *visibleBounds)) {
        std::cerr << "Failed to create Box texture from: " << path->string() << '\n';
        return nullptr;
    }
    texture->setSmooth(true);
    const sf::Texture* result = texture.get();
    ownedTextures_.push_back(std::move(texture));
    return result;
}

const sf::Texture* ResourceManager::loadHeroTexture(PieceColor color) {
    const std::filesystem::path workingDirectory = std::filesystem::current_path();
    const std::vector<std::filesystem::path> searchDirectories{
        workingDirectory / "assets" / "heroes",
        workingDirectory.parent_path() / "assets" / "heroes",
    };
    const auto path = findHeroTextureFile(color, searchDirectories);
    if (!path.has_value()) {
        std::cerr << "Failed to load hero texture: assets/heroes/"
                  << heroTextureFilename(color) << " (file not found)\n";
        return nullptr;
    }

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(*path)) {
        std::cerr << "Failed to load hero texture: " << path->string() << '\n';
        return nullptr;
    }
    texture->setSmooth(true);
    const sf::Texture* result = texture.get();
    ownedTextures_.push_back(std::move(texture));
    return result;
}

const sf::Texture* ResourceManager::loadHudTexture(HudIconKind kind) {
    const std::filesystem::path workingDirectory = std::filesystem::current_path();
    const std::filesystem::path resourceDirectory{hudTextureDirectoryName(kind)};
    const std::vector<std::filesystem::path> searchDirectories{
        workingDirectory / "assets" / resourceDirectory,
        workingDirectory.parent_path() / "assets" / resourceDirectory,
    };
    const auto path = findHudTextureFile(kind, searchDirectories);
    if (!path.has_value()) {
        std::string message = "HUD texture not found: ";
        message += hudTextureFilename(kind);
        message += " (searched";
        for (const auto& directory : searchDirectories) {
            message += " ";
            message += std::filesystem::absolute(directory / hudTextureFilename(kind))
                           .lexically_normal()
                           .string();
        }
        message += ")";
        throw std::runtime_error(message);
    }

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(*path)) {
        throw std::runtime_error(
            "Failed to load HUD texture: " +
            std::filesystem::absolute(*path).lexically_normal().string());
    }
    texture->setSmooth(true);
    const sf::Texture* result = texture.get();
    ownedTextures_.push_back(std::move(texture));
    return result;
}

void ResourceManager::loadBackgroundTexture() {
    const std::filesystem::path workingDirectory = std::filesystem::current_path();
    const std::array candidates{
        workingDirectory / "assets" / "backgrounds" / "battle_parchment.png",
        workingDirectory.parent_path() / "assets" / "backgrounds" / "battle_parchment.png",
    };

    for (const auto& path : candidates) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error) || error) {
            continue;
        }
        auto texture = std::make_unique<sf::Texture>();
        if (texture->loadFromFile(path)) {
            texture->setSmooth(true);
            backgroundTexture_ = std::move(texture);
            return;
        }
        std::cerr << "Failed to load battle background: " << path.string() << '\n';
        return;
    }

    std::cerr << "Battle background not found: assets/backgrounds/battle_parchment.png\n";
}

} // namespace sts
