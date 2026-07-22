#pragma once

#include <compare>

namespace sts {

struct Position {
    int row{};
    int column{};

    auto operator<=>(const Position&) const = default;
};

} // namespace sts
