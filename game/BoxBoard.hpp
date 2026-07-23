#pragma once

#include "common/BoardDimensions.hpp"
#include "common/Protocol.hpp"
#include "game/BoardPiece.hpp"

#include <array>
#include <memory>
#include <optional>
#include <random>
#include <vector>

namespace sts {

class Hero;

struct EliminationValueResult {
    int boxCount{};
    int inheritedHeroValue{};
    int totalAttributeValue{};
};

struct DeadHeroInfo {
    HeroId id{};
    PieceColor color{PieceColor::Red};
    Position position{};
};

class BoxBoard {
public:
    static constexpr int RowCount = BoardRows;
    static constexpr int ColumnCount = BoardColumns;
    using Storage = std::array<std::array<std::unique_ptr<BoardPiece>, ColumnCount>, RowCount>;

    BoxBoard();
    explicit BoxBoard(std::uint32_t seed);
    void initializeRandom();

    [[nodiscard]] const BoardPiece* pieceAt(Position position) const noexcept;
    [[nodiscard]] BoardPiece* pieceAt(Position position) noexcept;
    [[nodiscard]] bool isInside(Position position) const noexcept;
    [[nodiscard]] BoardSnapshot snapshot() const;

    // This is intentionally useful for deterministic tests and future board loading.
    void setPieceAt(Position position, std::unique_ptr<BoardPiece> piece);
    [[nodiscard]] std::unique_ptr<BoardPiece> generateBoxAt(Position position);
    [[nodiscard]] EliminationValueResult calculateEliminationValue(const std::vector<Position>& path) const;
    [[nodiscard]] HeroId resolveElimination(int actingPlayerId, const std::vector<Position>& path);
    void collapseAndRefill(int actingPlayerId, const std::vector<Position>& eliminatedPath);
    [[nodiscard]] Hero* heroById(HeroId id) noexcept;
    [[nodiscard]] const Hero* heroById(HeroId id) const noexcept;
    [[nodiscard]] std::vector<HeroId> livingHeroIds() const;
    [[nodiscard]] std::vector<HeroId> livingHeroIdsForPlayer(int playerId) const;
    [[nodiscard]] std::vector<DeadHeroInfo> convertDeadHeroesToBoxes();

private:
    void collapseColumnUp(int column);
    void collapseColumnDown(int column);

    Storage cells_{};
    std::mt19937 random_;
    HeroId nextHeroId_{1};
};

} // namespace sts
