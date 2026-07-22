#pragma once

#include "game/BoardPiece.hpp"

#include <memory>
#include <string>

namespace sts {

struct ImageResource {
    std::string id;
};

class Box final : public BoardPiece {
public:
    Box(PieceColor color, Position position);

    [[nodiscard]] PieceType type() const noexcept override { return PieceType::Box; }
    [[nodiscard]] int inheritedAttribute() const noexcept override { return 0; }
    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] const std::shared_ptr<const ImageResource>& image() const noexcept { return image_; }

    void onSpawn() override;
    void onEliminated() override;
    [[nodiscard]] bool canConnectTo(const BoardPiece& other) const noexcept;

private:
    std::shared_ptr<const ImageResource> image_;
    bool active_{true};
    unsigned int spawnEvents_{};
    unsigned int eliminationEvents_{};
};

} // namespace sts
