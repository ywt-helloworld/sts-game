#pragma once

#include "common/BoardDimensions.hpp"
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
    int inheritedAttribute{};

    bool operator==(const PieceSnapshot&) const = default;
};

using BoardSnapshot = std::array<std::array<PieceSnapshot, BoardColumns>, BoardRows>;

struct EliminateRequest {
    int playerId{};
    int turnId{};
    std::vector<Position> path;
};

struct EliminateResult {
    bool success{};
    std::string errorMessage;
    int nextPlayerId{};
    int nextTurnId{};
    BoardSnapshot board{};
};

struct GameStartedMessage {
    int currentPlayerId{};
    int turnId{};
    BoardSnapshot board{};
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
