#include "common/Protocol.hpp"
#include "server/GameServer.hpp"
#include "tests/TestHarness.hpp"

#include <asio.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace {

using namespace sts;
using namespace std::chrono_literals;

class RunningServer {
public:
    RunningServer() : server_(0), thread_([this] { server_.run(); }) {}
    ~RunningServer() {
        server_.stop();
        thread_.join();
    }

    [[nodiscard]] std::uint16_t port() const { return server_.port(); }

private:
    GameServer server_;
    std::thread thread_;
};

class Peer {
public:
    explicit Peer(std::uint16_t port) : socket_(context_) {
        socket_.connect({asio::ip::make_address("127.0.0.1"), port});
    }

    ~Peer() { close(); }

    [[nodiscard]] std::string receive() {
        std::array<std::byte, 4> header{};
        asio::read(socket_, asio::buffer(header));
        const auto length = decodeFrameLength(header);
        if (!length.has_value()) {
            throw std::runtime_error("invalid frame header");
        }
        std::string body(*length, '\0');
        asio::read(socket_, asio::buffer(body));
        return body;
    }

    void send(const std::string& body) {
        const std::vector<std::byte> frame = makeLengthPrefixedFrame(body);
        asio::write(socket_, asio::buffer(frame));
    }

    [[nodiscard]] bool hasNoPendingMessage() {
        std::this_thread::sleep_for(100ms);
        std::error_code error;
        return socket_.available(error) == 0U && !error;
    }

    void close() noexcept {
        std::error_code ignored;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    }

private:
    asio::io_context context_;
    asio::ip::tcp::socket socket_;
};

void expectJoin(Peer& peer, int playerId) {
    const auto message = deserializeWireMessage(peer.receive());
    REQUIRE(message.has_value());
    REQUIRE(message->type == MessageType::JoinRoom);
    REQUIRE(message->payload == std::to_string(playerId));
}

void expectWire(Peer& peer, MessageType type) {
    const auto message = deserializeWireMessage(peer.receive());
    REQUIRE(message.has_value());
    REQUIRE(message->type == type);
}

GameStartedMessage expectGameStarted(Peer& peer) {
    const auto message = deserializeGameStartedMessage(peer.receive());
    REQUIRE(message.has_value());
    REQUIRE(message->currentPlayerId == 0);
    REQUIRE(message->turnId == 1);
    return *message;
}

std::vector<Position> findPlayerZeroPath(const BoardSnapshot& board) {
    constexpr std::array<Position, 8> directions{{{-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
                                                   {0, 1}, {1, -1}, {1, 0}, {1, 1}}};
    for (int row = PlayerAreaRows; row < BoardRows; ++row) {
        for (int column = 0; column < BoardColumns; ++column) {
            const Position first{row, column};
            for (const Position firstStep : directions) {
                const Position second{first.row + firstStep.row, first.column + firstStep.column};
                if (second.row < PlayerAreaRows || second.row >= BoardRows || second.column < 0 || second.column >= BoardColumns ||
                    board[first.row][first.column].color != board[second.row][second.column].color) {
                    continue;
                }
                for (const Position secondStep : directions) {
                    const Position third{second.row + secondStep.row, second.column + secondStep.column};
                    if (third == first || third.row < PlayerAreaRows || third.row >= BoardRows || third.column < 0 || third.column >= BoardColumns) {
                        continue;
                    }
                    if (board[first.row][first.column].color == board[third.row][third.column].color) {
                        return {first, second, third};
                    }
                }
            }
        }
    }
    throw std::runtime_error("the deterministic board unexpectedly has no player-zero path");
}

void testDisconnectBeforeStartReusesPlayerZero() {
    RunningServer server;
    {
        Peer first(server.port());
        expectJoin(first, 0);
        expectWire(first, MessageType::PlayerReady);
        first.close();
    }
    std::this_thread::sleep_for(100ms);

    Peer replacement(server.port());
    expectJoin(replacement, 0);
    expectWire(replacement, MessageType::PlayerReady);
    REQUIRE(replacement.hasNoPendingMessage());
}

void testTwoLivePlayersStartAndThirdGetsRoomFull() {
    RunningServer server;
    Peer player0(server.port());
    expectJoin(player0, 0);
    expectWire(player0, MessageType::PlayerReady);

    Peer player1(server.port());
    expectJoin(player1, 1);
    const GameStartedMessage player0Started = expectGameStarted(player0);
    const GameStartedMessage player1Started = expectGameStarted(player1);
    REQUIRE(player0Started.board == player1Started.board);

    Peer third(server.port());
    const auto message = deserializeWireMessage(third.receive());
    REQUIRE(message.has_value());
    REQUIRE(message->type == MessageType::Error);
    REQUIRE(message->payload == "RoomFull");
}

void testDisconnectAfterStartNotifiesOpponent() {
    RunningServer server;
    Peer player0(server.port());
    expectJoin(player0, 0);
    expectWire(player0, MessageType::PlayerReady);
    Peer player1(server.port());
    expectJoin(player1, 1);
    const GameStartedMessage player0Started = expectGameStarted(player0);
    const GameStartedMessage player1Started = expectGameStarted(player1);
    REQUIRE(player0Started.board == player1Started.board);

    player1.close();
    expectWire(player0, MessageType::OpponentDisconnected);
    expectWire(player0, MessageType::GameEnded);
}

void testBothPlayersReceiveTheSameBoardUpdate() {
    RunningServer server;
    Peer player0(server.port());
    expectJoin(player0, 0);
    expectWire(player0, MessageType::PlayerReady);
    Peer player1(server.port());
    expectJoin(player1, 1);
    const GameStartedMessage player0Started = expectGameStarted(player0);
    const GameStartedMessage player1Started = expectGameStarted(player1);
    REQUIRE(player0Started.board == player1Started.board);

    player0.send(serialize(EliminateRequest{0, 1, findPlayerZeroPath(player0Started.board)}));
    const auto player0Result = deserializeEliminateResult(player0.receive());
    const auto player1Result = deserializeEliminateResult(player1.receive());
    REQUIRE(player0Result.has_value());
    REQUIRE(player1Result.has_value());
    REQUIRE(player0Result->success);
    REQUIRE(player1Result->success);
    REQUIRE(player0Result->board == player1Result->board);
}

} // namespace

int main() {
    try {
        testDisconnectBeforeStartReusesPlayerZero();
        testTwoLivePlayersStartAndThirdGetsRoomFull();
        testDisconnectAfterStartNotifiesOpponent();
        testBothPlayersReceiveTheSameBoardUpdate();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
