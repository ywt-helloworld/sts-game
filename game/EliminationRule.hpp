#pragma once

#include "common/Position.hpp"

#include <string>
#include <vector>

namespace sts {

class BoxBoard;

struct ValidationResult {
    bool valid{};
    std::string errorMessage;
};

class EliminationRule {
public:
    [[nodiscard]] ValidationResult validate(const BoxBoard& board, int playerId,
                                            const std::vector<Position>& path) const;

private:
    [[nodiscard]] static bool isInPlayerArea(int playerId, Position position) noexcept;
};

} // namespace sts
