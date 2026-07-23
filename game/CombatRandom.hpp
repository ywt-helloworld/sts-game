#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace sts {

class CombatRandom {
public:
    explicit CombatRandom(std::uint32_t seed);
    explicit CombatRandom(std::vector<std::size_t> scriptedIndices);

    [[nodiscard]] std::size_t chooseIndex(std::size_t candidateCount);

private:
    std::mt19937 engine_;
    std::vector<std::size_t> scriptedIndices_;
    std::size_t nextScriptedIndex_{};
};

} // namespace sts
