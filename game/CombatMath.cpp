#include "game/CombatMath.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace sts {
namespace {

int checkedInt(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        throw std::overflow_error("combat integer calculation overflowed");
    }
    return static_cast<int>(value);
}

} // namespace

int checkedMultiply(int left, int right) {
    return checkedInt(static_cast<std::int64_t>(left) * static_cast<std::int64_t>(right));
}

int checkedAdd(int left, int right) {
    return checkedInt(static_cast<std::int64_t>(left) + static_cast<std::int64_t>(right));
}

int roundPositiveRatio(int value, int numerator, int denominator) {
    if (value < 0 || numerator < 0 || denominator <= 0) {
        throw std::invalid_argument("roundPositiveRatio requires non-negative values and a positive denominator");
    }
    const std::int64_t product = static_cast<std::int64_t>(value) * numerator;
    const std::int64_t rounded = (product + denominator / 2) / denominator;
    return checkedInt(rounded);
}

} // namespace sts
