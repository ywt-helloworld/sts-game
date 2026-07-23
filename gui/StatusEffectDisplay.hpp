#pragma once

#include "common/Protocol.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace sts {

enum class HudIconKind {
    Vulnerable,
    Weak,
    Shield,
    GloryStar,
    LightningChargeOrb
};

struct StatusDisplayItem {
    HudIconKind kind{HudIconKind::Vulnerable};
    int value{};

    bool operator==(const StatusDisplayItem&) const = default;
};

struct StatusStripMetrics {
    float iconSize{};
    float panelSize{};
    float gap{};
    float totalWidth{};
    float height{};

    bool operator==(const StatusStripMetrics&) const = default;
};

[[nodiscard]] std::string_view hudTextureFilename(HudIconKind kind) noexcept;
[[nodiscard]] std::string_view hudTextureDirectoryName(HudIconKind kind) noexcept;
[[nodiscard]] std::optional<std::filesystem::path>
findHudTextureFile(HudIconKind kind,
                   const std::vector<std::filesystem::path>& searchDirectories) noexcept;

[[nodiscard]] std::vector<StatusDisplayItem>
heroStatusDisplayItems(const PieceSnapshot& hero);
[[nodiscard]] std::vector<StatusDisplayItem>
towerStatusDisplayItems(const TowerSnapshot& tower);
[[nodiscard]] std::vector<StatusDisplayItem>
heroResourceDisplayItems(const PieceSnapshot& hero);

[[nodiscard]] float statusIconSize(float referenceWidth) noexcept;
[[nodiscard]] StatusStripMetrics statusStripMetrics(std::size_t itemCount,
                                                    float referenceWidth) noexcept;

} // namespace sts
