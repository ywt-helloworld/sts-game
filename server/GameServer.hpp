#pragma once

#include "game/GameSession.hpp"

#include <asio.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace sts {

class GameServer {
public:
    explicit GameServer(std::uint16_t port);
    ~GameServer();

    GameServer(const GameServer&) = delete;
    GameServer& operator=(const GameServer&) = delete;

    void run();
    void stop() noexcept;
    [[nodiscard]] std::uint16_t port() const;

private:
    using Tcp = asio::ip::tcp;

    class ClientSession : public std::enable_shared_from_this<ClientSession> {
    public:
        ClientSession(Tcp::socket socket, GameServer& server, int playerId);
        ~ClientSession() = default;

        ClientSession(const ClientSession&) = delete;
        ClientSession& operator=(const ClientSession&) = delete;

        void start();
        void send(std::string body);
        void sendAndClose(std::string body);
        void disconnect() noexcept;
        [[nodiscard]] int playerId() const noexcept { return playerId_; }
        [[nodiscard]] bool isConnected() const noexcept;

    private:
        void readHeader();
        void readBody(std::uint32_t length);
        void writeNext();
        void disconnectOnce() noexcept;

        Tcp::socket socket_;
        GameServer& server_;
        int playerId_;
        bool disconnected_{};
        bool closeAfterWrite_{};
        std::array<std::byte, 4> header_{};
        std::string body_;
        std::deque<std::vector<std::byte>> writeQueue_;
    };

    void acceptNext();
    void onConnected(const std::shared_ptr<ClientSession>& client);
    void onMessage(const std::shared_ptr<ClientSession>& client, const std::string& body);
    void onClientDisconnected(const std::shared_ptr<ClientSession>& client) noexcept;
    void broadcast(const std::string& body);
    [[nodiscard]] int firstAvailablePlayerSlot() const noexcept;
    [[nodiscard]] bool hasTwoLivePlayers() const noexcept;
    [[nodiscard]] bool owns(const std::shared_ptr<ClientSession>& client) const noexcept;

    asio::io_context ioContext_;
    Tcp::acceptor acceptor_;
    GameSession game_;
    std::array<std::shared_ptr<ClientSession>, 2> players_{};
    bool stopped_{};
};

} // namespace sts
