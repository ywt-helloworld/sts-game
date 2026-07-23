#include "gui/BoxTextureCatalog.hpp"

#include <array>
#include <system_error>

namespace sts {
namespace {

constexpr std::array normalFiles{
    std::string_view{"normal_red.png"},
    std::string_view{"normal_yellow.png"},
    std::string_view{"normal_green.png"},
    std::string_view{"normal_blue.png"},
};

constexpr std::array selectedFiles{
    std::string_view{"selected_red.png"},
    std::string_view{"selected_yellow.png"},
    std::string_view{"selected_green.png"},
    std::string_view{"selected_blue.png"},
};

std::size_t colorIndex(PieceColor color) noexcept {
    const auto index = static_cast<std::size_t>(color);
    return index < normalFiles.size() ? index : 0U;
}

} // namespace

std::string_view boxTextureFilename(PieceColor color, BoxTextureState state) noexcept {
    const std::size_t index = colorIndex(color);
    return state == BoxTextureState::Selected ? selectedFiles[index] : normalFiles[index];
}

std::optional<std::filesystem::path>
findBoxTextureFile(PieceColor color,
                   BoxTextureState state,
                   const std::vector<std::filesystem::path>& searchDirectories) noexcept {
    const std::filesystem::path filename{boxTextureFilename(color, state)};
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
