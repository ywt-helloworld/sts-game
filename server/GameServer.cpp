#include "server/GameServer.hpp"

#include "common/Protocol.hpp"

#include <iostream>
#include <system_error>
#include <utility>

namespace sts {
namespace {

constexpr std::uint32_t MaximumMessageBytes = 64U * 1024U;

std::string wire(MessageType type, std::string text) {
    return serializeWireMessage({type, std::move(text)});
}

} // namespace

GameServer::GameServer(std::uint16_t port)
    : acceptor_(ioContext_, Tcp::endpoint(Tcp::v4(), port)), game_(42) {}

GameServer::~GameServer() {
    stop();
}

void GameServer::run() {
    acceptNext();
    std::cout << "Server listening on port " << port() << "\n";
    ioContext_.run();
}

void GameServer::stop() noexcept {
    if (stopped_) {
        return;
    }
    stopped_ = true;
    std::error_code ignored;
    acceptor_.close(ignored);
    for (const auto& player : players_) {
        if (player != nullptr) {
            player->disconnect();
        }
    }
    ioContext_.stop();
}

std::uint16_t GameServer::port() const {
    std::error_code error;
    const auto endpoint = acceptor_.local_endpoint(error);
    return error ? 0 : endpoint.port();
}

void GameServer::acceptNext() {
    if (stopped_) {
        return;
    }
    acceptor_.async_accept([this](const std::error_code& error, Tcp::socket socket) {
        if (!error && !stopped_) {
            const int playerId = firstAvailablePlayerSlot();
            auto client = std::make_shared<ClientSession>(std::move(socket), *this, playerId);
            onConnected(client);
        }
        if (!stopped_) {
            acceptNext();
        }
    });
}

void GameServer::onConnected(const std::shared_ptr<ClientSession>& client) {
    if (client->playerId() < 0) {
        client->sendAndClose(wire(MessageType::Error, "RoomFull"));
        return;
    }
    if (game_.phase() == GamePhase::GameOver) {
        client->sendAndClose(wire(MessageType::GameEnded, "the current game has ended"));
        return;
    }

    players_[static_cast<std::size_t>(client->playerId())] = client;
    client->send(wire(MessageType::JoinRoom, std::to_string(client->playerId())));
    static_cast<void>(game_.markPlayerReady(client->playerId()));
    client->start();

    if (!hasTwoLivePlayers()) {
        client->send(wire(MessageType::PlayerReady, "waiting for the other player"));
        return;
    }
    if (!game_.start()) {
        broadcast(wire(MessageType::Error, "unable to start game"));
        return;
    }

    const GameStartedMessage started{game_.currentPlayerId(), game_.turnId(), game_.board().snapshot()};
    broadcast(serialize(started));
}

void GameServer::onMessage(const std::shared_ptr<ClientSession>& client, const std::string& body) {
    if (!owns(client)) {
        return;
    }
    const auto request = deserializeEliminateRequest(body);
    if (!request.has_value()) {
        client->send(wire(MessageType::Error, "expected a valid EliminateRequest message"));
        return;
    }
    if (request->playerId != client->playerId()) {
        client->send(wire(MessageType::Error, "the request playerId does not match this connection"));
        return;
    }

    const EliminateResult result = game_.handleEliminateRequest(*request);
    broadcast(serialize(result));
}

void GameServer::onClientDisconnected(const std::shared_ptr<ClientSession>& client) noexcept {
    if (!owns(client)) {
        return;
    }
    const int playerId = client->playerId();
    const bool wasPlaying = game_.phase() == GamePhase::Playing || game_.phase() == GamePhase::ResolvingTurn;
    players_[static_cast<std::size_t>(playerId)].reset();
    game_.markPlayerDisconnected(playerId);

    if (wasPlaying) {
        broadcast(wire(MessageType::OpponentDisconnected, "opponent disconnected"));
        broadcast(wire(MessageType::GameEnded, "game ended because the opponent disconnected"));
    } else if (game_.phase() == GamePhase::WaitingForPlayers) {
        broadcast(wire(MessageType::PlayerReady, "waiting for the other player"));
    }
}

void GameServer::broadcast(const std::string& body) {
    for (const auto& player : players_) {
        if (player != nullptr && player->isConnected()) {
            player->send(body);
        }
    }
}

int GameServer::firstAvailablePlayerSlot() const noexcept {
    for (int playerId = 0; playerId < static_cast<int>(players_.size()); ++playerId) {
        const auto& player = players_[static_cast<std::size_t>(playerId)];
        if (player == nullptr || !player->isConnected()) {
            return playerId;
        }
    }
    return -1;
}

bool GameServer::hasTwoLivePlayers() const noexcept {
    return players_[0] != nullptr && players_[1] != nullptr &&
           players_[0]->isConnected() && players_[1]->isConnected();
}

bool GameServer::owns(const std::shared_ptr<ClientSession>& client) const noexcept {
    const int playerId = client->playerId();
    return playerId >= 0 && playerId < static_cast<int>(players_.size()) &&
           players_[static_cast<std::size_t>(playerId)] == client;
}

GameServer::ClientSession::ClientSession(Tcp::socket socket, GameServer& server, int playerId)
    : socket_(std::move(socket)), server_(server), playerId_(playerId) {}

void GameServer::ClientSession::start() {
    if (!disconnected_) {
        readHeader();
    }
}

void GameServer::ClientSession::send(std::string body) {
    if (disconnected_) {
        return;
    }
    const bool currentlyWriting = !writeQueue_.empty();
    writeQueue_.push_back(makeLengthPrefixedFrame(body));
    if (!currentlyWriting) {
        writeNext();
    }
}

void GameServer::ClientSession::sendAndClose(std::string body) {
    if (disconnected_) {
        return;
    }
    closeAfterWrite_ = true;
    send(std::move(body));
}

void GameServer::ClientSession::disconnect() noexcept {
    disconnectOnce();
}

bool GameServer::ClientSession::isConnected() const noexcept {
    return !disconnected_ && socket_.is_open();
}

void GameServer::ClientSession::readHeader() {
    auto self = shared_from_this();
    asio::async_read(socket_, asio::buffer(header_), [self](const std::error_code& error, std::size_t) {
        if (error) {
            self->disconnectOnce();
            return;
        }
        const auto length = decodeFrameLength(self->header_);
        if (!length.has_value() || *length > MaximumMessageBytes) {
            self->sendAndClose(wire(MessageType::Error, "invalid message length"));
            return;
        }
        self->readBody(*length);
    });
}

void GameServer::ClientSession::readBody(std::uint32_t length) {
    body_.assign(length, '\0');
    auto self = shared_from_this();
    asio::async_read(socket_, asio::buffer(body_), [self](const std::error_code& error, std::size_t) {
        if (error) {
            self->disconnectOnce();
            return;
        }
        self->server_.onMessage(self, self->body_);
        if (!self->disconnected_) {
            self->readHeader();
        }
    });
}

void GameServer::ClientSession::writeNext() {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(writeQueue_.front()), [self](const std::error_code& error, std::size_t) {
        if (error) {
            self->disconnectOnce();
            return;
        }
        self->writeQueue_.pop_front();
        if (!self->writeQueue_.empty()) {
            self->writeNext();
        } else if (self->closeAfterWrite_) {
            self->disconnectOnce();
        }
    });
}

void GameServer::ClientSession::disconnectOnce() noexcept {
    if (disconnected_) {
        return;
    }
    disconnected_ = true;
    std::error_code ignored;
    socket_.shutdown(Tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
    server_.onClientDisconnected(shared_from_this());
}

} // namespace sts
