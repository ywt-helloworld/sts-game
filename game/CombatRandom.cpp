#include "game/CombatRandom.hpp"

#include <stdexcept>
#include <utility>

namespace sts {

CombatRandom::CombatRandom(std::uint32_t seed) : engine_(seed) {}

CombatRandom::CombatRandom(std::vector<std::size_t> scriptedIndices)
    : engine_(0), scriptedIndices_(std::move(scriptedIndices)) {}

std::size_t CombatRandom::chooseIndex(std::size_t candidateCount) {
    if (candidateCount == 0U) {
        throw std::invalid_argument("cannot choose from an empty combat target list");
    }
    if (nextScriptedIndex_ < scriptedIndices_.size()) {
        return scriptedIndices_[nextScriptedIndex_++] % candidateCount;
    }
    std::uniform_int_distribution<std::size_t> distribution(0U, candidateCount - 1U);
    return distribution(engine_);
}

} // namespace sts
