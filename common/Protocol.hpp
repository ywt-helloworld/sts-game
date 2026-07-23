#pragma once

#include "common/BoardDimensions.hpp"
#include "common/GameTypes.hpp"
#include "common/PieceColor.hpp"
#include "common/Position.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sts {

enum class MessageType : std::uint8_t {
    CreateRoom,
    JoinRoom,
    PlayerReady,
    GameStarted,
    EliminateRequest,
    EliminateResult,
    BoardUpdated,
    TurnChanged,
    GameEnded,
    OpponentDisconnected,
    Error
};

struct PieceSnapshot {
    PieceType type{PieceType::Box};
    PieceColor color{PieceColor::Red};
    Position position{};
    int attributeValue{};
    HeroId heroId{};
    HeroType heroType{HeroType::None};
    int currentHp{};
    int maxHp{};
    int currentBaseAttackDamage{};
    int defense{};
    int shield{};
    int vulnerableLayers{};
    int weakLayers{};
    int radiantStars{};
    int lightningOrbs{};
    bool alive{true};

    bool operator==(const PieceSnapshot&) const = default;
};

using BoardSnapshot = std::array<std::array<PieceSnapshot, BoardColumns>, BoardRows>;

struct GameSnapshot {
    GamePhase phase{GamePhase::WaitingForPlayers};
    int currentPlayerId{};
    int turnId{1};
    std::optional<int> winnerPlayerId;
    std::array<TowerSnapshot, 2> towers{};
    BoardSnapshot board{};
    bool openingTurnPending{true};

    bool operator==(const GameSnapshot&) const = default;
};

struct EliminateRequest {
    int playerId{};
    int turnId{};
    std::vector<Position> path;
};

struct EliminateResult {
    bool success{};
    std::string errorMessage;
    GameSnapshot game{};
    std::vector<CombatEvent> combatEvents;
};

struct GameStartedMessage {
    GameSnapshot game{};
};

struct WireMessage {
    MessageType type{MessageType::Error};
    std::string payload;
};

std::string serialize(const EliminateRequest& request);
std::optional<EliminateRequest> deserializeEliminateRequest(const std::string& body);
std::string serialize(const EliminateResult& result);
std::optional<EliminateResult> deserializeEliminateResult(const std::string& body);
std::string serialize(const GameStartedMessage& message);
std::optional<GameStartedMessage> deserializeGameStartedMessage(const std::string& body);
std::string serializeWireMessage(WireMessage message);
std::optional<WireMessage> deserializeWireMessage(const std::string& body);

// TCP is a byte stream; callers must write all bytes returned here and read the
// four-byte big-endian length before reading the body.
std::vector<std::byte> makeLengthPrefixedFrame(const std::string& body);
std::optional<std::uint32_t> decodeFrameLength(const std::array<std::byte, 4>& header);

} // namespace sts
