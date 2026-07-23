#pragma once

#include "common/PieceColor.hpp"

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace sts {

struct HeroVisualLayout {
    float left{};
    float top{};
    float width{};
    float height{};
    float uniformScale{};
};

[[nodiscard]] std::string_view heroTextureFilename(PieceColor color) noexcept;
[[nodiscard]] constexpr bool pieceUsesHeroTexture(PieceType type) noexcept {
    return type == PieceType::Hero;
}
[[nodiscard]] constexpr float heroSelectionScale(bool selected) noexcept {
    return selected ? 1.07F : 1.0F;
}
[[nodiscard]] HeroVisualLayout fitHeroTextureInsideCell(unsigned textureWidth,
                                                        unsigned textureHeight,
                                                        float cellLeft,
                                                        float cellTop,
                                                        float cellWidth,
                                                        float cellHeight,
                                                        bool selected) noexcept;
[[nodiscard]] std::optional<std::filesystem::path>
findHeroTextureFile(PieceColor color,
                    const std::vector<std::filesystem::path>& searchDirectories) noexcept;

} // namespace sts
