#include "gui/StatusEffectDisplay.hpp"

#include <algorithm>
#include <array>
#include <system_error>

namespace sts {
namespace {

constexpr std::array hudFiles{
    std::string_view{"vulnerable.png"},
    std::string_view{"weak.png"},
    std::string_view{"shield.png"},
    std::string_view{"glory_star.png"},
    std::string_view{"lightning_charge_orb.png"},
};

std::size_t hudIndex(HudIconKind kind) noexcept {
    const auto index = static_cast<std::size_t>(kind);
    return index < hudFiles.size() ? index : 0U;
}

void appendPositive(std::vector<StatusDisplayItem>& items,
                    HudIconKind kind,
                    int value) {
    if (value > 0) {
        items.push_back({kind, value});
    }
}

} // namespace

std::string_view hudTextureFilename(HudIconKind kind) noexcept {
    return hudFiles[hudIndex(kind)];
}

std::string_view hudTextureDirectoryName(HudIconKind kind) noexcept {
    return kind == HudIconKind::GloryStar ||
                   kind == HudIconKind::LightningChargeOrb
               ? std::string_view{"resources"}
               : std::string_view{"status"};
}

std::optional<std::filesystem::path>
findHudTextureFile(HudIconKind kind,
                   const std::vector<std::filesystem::path>& searchDirectories) noexcept {
    const std::filesystem::path filename{hudTextureFilename(kind)};
    for (const auto& directory : searchDirectories) {
        const std::filesystem::path candidate = directory / filename;
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error) && !error) {
            return candidate;
        }
    }
    return std::nullopt;
}

std::vector<StatusDisplayItem>
heroStatusDisplayItems(const PieceSnapshot& hero) {
    std::vector<StatusDisplayItem> items;
    items.reserve(3U);
    appendPositive(items, HudIconKind::Vulnerable, hero.vulnerableLayers);
    appendPositive(items, HudIconKind::Weak, hero.weakLayers);
    appendPositive(items, HudIconKind::Shield, hero.shield);
    return items;
}

std::vector<StatusDisplayItem>
towerStatusDisplayItems(const TowerSnapshot& tower) {
    std::vector<StatusDisplayItem> items;
    items.reserve(2U);
    appendPositive(items, HudIconKind::Vulnerable, tower.vulnerableLayers);
    appendPositive(items, HudIconKind::Weak, tower.weakLayers);
    return items;
}

std::vector<StatusDisplayItem>
heroResourceDisplayItems(const PieceSnapshot& hero) {
    switch (hero.heroType) {
    case HeroType::Regent:
        return {{HudIconKind::GloryStar, hero.radiantStars}};
    case HeroType::ChickenPot:
        return {{HudIconKind::LightningChargeOrb, hero.lightningOrbs}};
    case HeroType::None:
    case HeroType::IronFighter:
    case HeroType::SilentHunter:
        return {};
    }
    return {};
}

float statusIconSize(float referenceWidth) noexcept {
    return std::clamp(referenceWidth * 0.18F, 16.0F, 24.0F);
}

StatusStripMetrics statusStripMetrics(std::size_t itemCount,
                                      float referenceWidth) noexcept {
    if (itemCount == 0U) {
        return {};
    }
    const float iconSize = statusIconSize(referenceWidth);
    const float padding = std::clamp(iconSize * 0.12F, 2.0F, 3.0F);
    const float panelSize = iconSize + padding * 2.0F;
    const float gap = std::clamp(iconSize * 0.10F, 2.0F, 3.0F);
    return {
        iconSize,
        panelSize,
        gap,
        panelSize * static_cast<float>(itemCount) +
            gap * static_cast<float>(itemCount - 1U),
        panelSize,
    };
}

} // namespace sts
