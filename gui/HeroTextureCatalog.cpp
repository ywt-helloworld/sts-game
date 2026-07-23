#include "gui/HeroTextureCatalog.hpp"

#include <algorithm>
#include <array>
#include <system_error>

namespace sts {
namespace {

constexpr std::array heroFiles{
    std::string_view{"hero_red.png"},
    std::string_view{"hero_yellow.png"},
    std::string_view{"hero_green.png"},
    std::string_view{"hero_blue.png"},
};

std::size_t colorIndex(PieceColor color) noexcept {
    const auto index = static_cast<std::size_t>(color);
    return index < heroFiles.size() ? index : 0U;
}

} // namespace

std::string_view heroTextureFilename(PieceColor color) noexcept {
    return heroFiles[colorIndex(color)];
}

HeroVisualLayout fitHeroTextureInsideCell(unsigned textureWidth,
                                          unsigned textureHeight,
                                          float cellLeft,
                                          float cellTop,
                                          float cellWidth,
                                          float cellHeight,
                                          bool selected) noexcept {
    if (textureWidth == 0U || textureHeight == 0U || cellWidth <= 0.0F || cellHeight <= 0.0F) {
        return {cellLeft, cellTop, 0.0F, 0.0F, 0.0F};
    }

    constexpr float maxWidthRatio = 0.96F;
    constexpr float maxHeightRatio = 1.05F;
    const float baseScale = std::min(cellWidth * maxWidthRatio / static_cast<float>(textureWidth),
                                     cellHeight * maxHeightRatio / static_cast<float>(textureHeight));
    const float scale = baseScale * heroSelectionScale(selected);
    const float width = static_cast<float>(textureWidth) * scale;
    const float height = static_cast<float>(textureHeight) * scale;
    return {cellLeft + (cellWidth - width) / 2.0F,
            cellTop + (cellHeight - height) / 2.0F,
            width,
            height,
            scale};
}

std::optional<std::filesystem::path>
findHeroTextureFile(PieceColor color,
                    const std::vector<std::filesystem::path>& searchDirectories) noexcept {
    const std::filesystem::path filename{heroTextureFilename(color)};
    for (const auto& directory : searchDirectories) {
        const std::filesystem::path candidate = directory / filename;
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error) && !error) {
            return candidate;
        }
    }
    return std::nullopt;
}

} // namespace sts
