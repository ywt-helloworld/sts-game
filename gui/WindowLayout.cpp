#include "gui/WindowLayout.hpp"

#include <algorithm>
#include <cmath>

namespace sts {
namespace {

unsigned scaledDimension(unsigned desktopDimension,
                         float ratio,
                         unsigned minimum,
                         unsigned maximum) noexcept {
    if (desktopDimension == 0U) {
        return 1U;
    }
    const auto safeDesktopMaximum =
        std::max(1U,
                 static_cast<unsigned>(
                     std::floor(static_cast<float>(desktopDimension) * 0.94F)));
    const auto preferred =
        static_cast<unsigned>(std::lround(static_cast<float>(desktopDimension) * ratio));
    return std::clamp(preferred,
                      std::min(minimum, safeDesktopMaximum),
                      std::min(maximum, safeDesktopMaximum));
}

ButtonLayout bottomButtonLayout(unsigned windowWidth,
                                unsigned windowHeight,
                                bool alignRight) noexcept {
    constexpr float horizontalMargin = 24.0F;
    constexpr float bottomMargin = 26.0F;
    constexpr float buttonWidth = 140.0F;
    constexpr float buttonHeight = 44.0F;
    const float availableWidth = static_cast<float>(windowWidth);
    const float availableHeight = static_cast<float>(windowHeight);
    const float left = alignRight
                           ? std::max(horizontalMargin,
                                      availableWidth - horizontalMargin - buttonWidth)
                           : horizontalMargin;
    const float top = std::max(12.0F,
                               availableHeight - bottomMargin - buttonHeight);
    return {left, top, buttonWidth, buttonHeight};
}

} // namespace

WindowPlacement fitWindowInsideDesktop(unsigned desktopWidth,
                                       unsigned desktopHeight) noexcept {
    const unsigned width = scaledDimension(desktopWidth, 0.86F, 960U, 1800U);
    const unsigned height = scaledDimension(desktopHeight, 0.86F, 640U, 1100U);
    return {
        width,
        height,
        static_cast<int>((desktopWidth - width) / 2U),
        static_cast<int>((desktopHeight - height) / 2U),
    };
}

ButtonLayout clearButtonLayout(unsigned windowWidth,
                               unsigned windowHeight) noexcept {
    return bottomButtonLayout(windowWidth, windowHeight, false);
}

ButtonLayout confirmButtonLayout(unsigned windowWidth,
                                 unsigned windowHeight) noexcept {
    return bottomButtonLayout(windowWidth, windowHeight, true);
}

} // namespace sts
