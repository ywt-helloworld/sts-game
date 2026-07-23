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
    REQUIRE(message->game.currentPlayerId == 0);
    REQUIRE(message->game.turnId == 1);
    REQUIRE(message->game.phase == GamePhase::Elimination);
    REQUIRE(message->game.openingTurnPending);
    REQUIRE(message->game.towers[0].currentHp == 1000);
    REQUIRE(message->game.towers[1].currentHp == 1000);
    return *message;
}

std::vector<Position> findPlayerPath(const BoardSnapshot& board, int playerId) {
    constexpr std::array<Position, 8> directions{{{-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
                                                   {0, 1}, {1, -1}, {1, 0}, {1, 1}}};
    const int firstRow = playerId == 0 ? PlayerAreaRows : 0;
    const int rowLimit = playerId == 0 ? BoardRows : PlayerAreaRows;
    for (int row = firstRow; row < rowLimit; ++row) {
        for (int column = 0; column < BoardColumns; ++column) {
            const Position first{row, column};
            for (const Position firstStep : directions) {
                const Position second{first.row + firstStep.row, first.column + firstStep.column};
                if (second.row < firstRow || second.row >= rowLimit || second.column < 0 || second.column >= BoardColumns ||
                    board[first.row][first.column].color != board[second.row][second.column].color) {
                    continue;
                }
                for (const Position secondStep : directions) {
                    const Position third{second.row + secondStep.row, second.column + secondStep.column};
                    if (third == first || third.row < firstRow || third.row >= rowLimit || third.column < 0 || third.column >= BoardColumns) {
                        continue;
                    }
                    if (board[first.row][first.column].color == board[third.row][third.column].color) {
                        return {first, second, third};
                    }
                }
            }
        }
    }
    throw std::runtime_error("the deterministic board unexpectedly has no path for the requested player");
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
    REQUIRE(player0Started.game == player1Started.game);

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
    REQUIRE(player0Started.game == player1Started.game);

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
    REQUIRE(player0Started.game == player1Started.game);

    player0.send(serialize(EliminateRequest{0, 1, findPlayerPath(player0Started.game.board, 0)}));
    const std::string player0Body = player0.receive();
    const std::string player1Body = player1.receive();
    const auto player0Result = deserializeEliminateResult(player0Body);
    const auto player1Result = deserializeEliminateResult(player1Body);
    REQUIRE(player0Result.has_value());
    REQUIRE(player1Result.has_value());
    REQUIRE(player0Result->success);
    REQUIRE(player1Result->success);
    REQUIRE(player0Result->game == player1Result->game);
    REQUIRE(player0Result->combatEvents == player1Result->combatEvents);
    REQUIRE(player0Result->game.phase == GamePhase::Elimination);
    REQUIRE(!player0Result->game.openingTurnPending);
    REQUIRE(player0Result->game.currentPlayerId == 1);
    REQUIRE(player0Result->game.turnId == 2);
    REQUIRE(!player0Result->combatEvents.empty());
    REQUIRE(player0Result->game.towers[0].currentHp == Tower::MaxHp);
    REQUIRE(player0Result->game.towers[1].currentHp == Tower::MaxHp);
    bool sawOpeningCompleted = false;
    for (const CombatEvent& event : player0Result->combatEvents) {
        REQUIRE(event.type != CombatEventType::CombatStarted);
        REQUIRE(event.type != CombatEventType::CombatFinished);
        REQUIRE(event.type != CombatEventType::HeroAttacked);
        REQUIRE(event.type != CombatEventType::TowerDamaged);
        sawOpeningCompleted = sawOpeningCompleted || event.type == CombatEventType::OpeningTurnCompleted;
    }
    REQUIRE(sawOpeningCompleted);

    player1.send(serialize(EliminateRequest{1, 2, findPlayerPath(player1Result->game.board, 1)}));
    const auto player0Second = deserializeEliminateResult(player0.receive());
    const auto player1Second = deserializeEliminateResult(player1.receive());
    REQUIRE(player0Second.has_value());
    REQUIRE(player1Second.has_value());
    REQUIRE(player0Second->success);
    REQUIRE(player0Second->game == player1Second->game);
    REQUIRE(player0Second->combatEvents == player1Second->combatEvents);
    REQUIRE(!player0Second->game.openingTurnPending);
    REQUIRE(player0Second->game.currentPlayerId == 0);
    REQUIRE(player0Second->game.turnId == 3);
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
