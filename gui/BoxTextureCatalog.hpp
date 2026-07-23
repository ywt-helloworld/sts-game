#pragma once

#include "common/PieceColor.hpp"

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace sts {

enum class BoxTextureState { Normal, Selected };

[[nodiscard]] std::string_view boxTextureFilename(PieceColor color, BoxTextureState state) noexcept;
[[nodiscard]] constexpr BoxTextureState boxTextureState(bool selected) noexcept {
    return selected ? BoxTextureState::Selected : BoxTextureState::Normal;
}
[[nodiscard]] constexpr float boxVisualScale(PieceColor color, BoxTextureState state) noexcept {
    if (state == BoxTextureState::Selected) {
        return 1.08F;
    }
    // Keep the yellow tile just below the common baseline to balance its broader
    // visible face after transparent margins have been cropped.
    return color == PieceColor::Yellow ? 0.99F : 1.0F;
}
[[nodiscard]] constexpr bool pieceUsesBoxTexture(PieceType type) noexcept {
    return type == PieceType::Box;
}
[[nodiscard]] std::optional<std::filesystem::path>
findBoxTextureFile(PieceColor color,
                   BoxTextureState state,
                   const std::vector<std::filesystem::path>& searchDirectories) noexcept;

} // namespace sts
