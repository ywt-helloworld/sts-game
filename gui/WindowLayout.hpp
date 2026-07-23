#pragma once

namespace sts {

struct WindowPlacement {
    unsigned width{};
    unsigned height{};
    int left{};
    int top{};

    bool operator==(const WindowPlacement&) const = default;
};

struct ButtonLayout {
    float left{};
    float top{};
    float width{};
    float height{};

    bool operator==(const ButtonLayout&) const = default;
};

[[nodiscard]] WindowPlacement fitWindowInsideDesktop(unsigned desktopWidth,
                                                     unsigned desktopHeight) noexcept;
[[nodiscard]] ButtonLayout clearButtonLayout(unsigned windowWidth,
                                             unsigned windowHeight) noexcept;
[[nodiscard]] ButtonLayout confirmButtonLayout(unsigned windowWidth,
                                               unsigned windowHeight) noexcept;

} // namespace sts
