#include "common/Protocol.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

namespace sts {
namespace {

template <typename T>
void writeSnapshot(std::ostream& output, const T& board) {
    for (const auto& row : board) {
        for (const auto& piece : row) {
            output << static_cast<int>(piece.type) << ' '
                   << static_cast<int>(piece.color) << ' '
                   << piece.position.row << ' ' << piece.position.column << ' '
                   << piece.inheritedAttribute << ' ';
        }
    }
}

bool readSnapshot(std::istream& input, BoardSnapshot& board) {
    for (auto& row : board) {
        for (auto& piece : row) {
            int type{};
            int color{};
            if (!(input >> type >> color >> piece.position.row >> piece.position.column >> piece.inheritedAttribute) ||
                type < static_cast<int>(PieceType::Box) || type > static_cast<int>(PieceType::Hero) ||
                color < static_cast<int>(PieceColor::Red) || color > static_cast<int>(PieceColor::Blue)) {
                return false;
            }
            piece.type = static_cast<PieceType>(type);
            piece.color = static_cast<PieceColor>(color);
        }
    }
    return true;
}

} // namespace

std::string serialize(const EliminateRequest& request) {
    std::ostringstream output;
    output << static_cast<int>(MessageType::EliminateRequest) << ' ' << request.playerId << ' '
           << request.turnId << ' ' << request.path.size();
    for (const Position position : request.path) {
        output << ' ' << position.row << ' ' << position.column;
    }
    return output.str();
}

std::optional<EliminateRequest> deserializeEliminateRequest(const std::string& body) {
    std::istringstream input(body);
    int type{};
    EliminateRequest request;
    std::size_t count{};
    if (!(input >> type >> request.playerId >> request.turnId >> count) ||
        type != static_cast<int>(MessageType::EliminateRequest) || count > 50U) {
        return std::nullopt;
    }
    request.path.resize(count);
    for (Position& position : request.path) {
        if (!(input >> position.row >> position.column)) {
            return std::nullopt;
        }
    }
    return request;
}

std::string serialize(const EliminateResult& result) {
    std::ostringstream output;
    output << static_cast<int>(MessageType::EliminateResult) << ' ' << result.success << ' '
           << result.nextPlayerId << ' ' << result.nextTurnId << ' ' << std::quoted(result.errorMessage) << ' ';
    writeSnapshot(output, result.board);
    return output.str();
}

std::optional<EliminateResult> deserializeEliminateResult(const std::string& body) {
    std::istringstream input(body);
    int type{};
    EliminateResult result;
    if (!(input >> type >> result.success >> result.nextPlayerId >> result.nextTurnId >> std::quoted(result.errorMessage)) ||
        type != static_cast<int>(MessageType::EliminateResult) || !readSnapshot(input, result.board)) {
        return std::nullopt;
    }
    return result;
}

std::string serialize(const GameStartedMessage& message) {
    std::ostringstream output;
    output << static_cast<int>(MessageType::GameStarted) << ' ' << message.currentPlayerId << ' '
           << message.turnId << ' ';
    writeSnapshot(output, message.board);
    return output.str();
}

std::optional<GameStartedMessage> deserializeGameStartedMessage(const std::string& body) {
    std::istringstream input(body);
    int type{};
    GameStartedMessage message;
    if (!(input >> type >> message.currentPlayerId >> message.turnId) ||
        type != static_cast<int>(MessageType::GameStarted) || !readSnapshot(input, message.board)) {
        return std::nullopt;
    }
    return message;
}

std::string serializeWireMessage(WireMessage message) {
    std::ostringstream output;
    output << static_cast<int>(message.type) << ' ' << std::quoted(message.payload);
    return output.str();
}

std::optional<WireMessage> deserializeWireMessage(const std::string& body) {
    std::istringstream input(body);
    int type{};
    WireMessage message;
    if (!(input >> type >> std::quoted(message.payload)) || type < 0 || type > static_cast<int>(MessageType::Error)) {
        return std::nullopt;
    }
    message.type = static_cast<MessageType>(type);
    return message;
}

std::vector<std::byte> makeLengthPrefixedFrame(const std::string& body) {
    if (body.size() > std::numeric_limits<std::uint32_t>::max()) {
        return {};
    }
    const auto length = static_cast<std::uint32_t>(body.size());
    std::vector<std::byte> frame(4U + body.size());
    frame[0] = static_cast<std::byte>((length >> 24U) & 0xffU);
    frame[1] = static_cast<std::byte>((length >> 16U) & 0xffU);
    frame[2] = static_cast<std::byte>((length >> 8U) & 0xffU);
    frame[3] = static_cast<std::byte>(length & 0xffU);
    for (std::size_t index = 0; index < body.size(); ++index) {
        frame[index + 4U] = static_cast<std::byte>(static_cast<unsigned char>(body[index]));
    }
    return frame;
}

std::optional<std::uint32_t> decodeFrameLength(const std::array<std::byte, 4>& header) {
    const auto value = (static_cast<std::uint32_t>(std::to_integer<unsigned char>(header[0])) << 24U) |
                       (static_cast<std::uint32_t>(std::to_integer<unsigned char>(header[1])) << 16U) |
                       (static_cast<std::uint32_t>(std::to_integer<unsigned char>(header[2])) << 8U) |
                       static_cast<std::uint32_t>(std::to_integer<unsigned char>(header[3]));
    return value;
}

} // namespace sts
