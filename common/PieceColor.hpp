#pragma once

#include <string_view>

namespace sts {

enum class PieceColor { Red, Yellow, Green, Blue };
enum class PieceType { Box, Hero };

inline constexpr std::string_view toString(PieceColor color) {
    switch (color) {
    case PieceColor::Red: return "red";
    case PieceColor::Yellow: return "yellow";
    case PieceColor::Green: return "green";
    case PieceColor::Blue: return "blue";
    }
    return "unknown";
}

inline constexpr std::string_view toString(PieceType type) {
    return type == PieceType::Box ? "box" : "hero";
}

} // namespace sts
