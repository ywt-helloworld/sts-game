#pragma once

#include "client/GameClient.hpp"
#include "gui/ThreadSafeMessageQueue.hpp"

#include <atomic>
#include <string>
#include <thread>
#include <variant>

namespace sts {

struct NetworkDisconnected {
    std::string error;
};

using GuiNetworkMessage = std::variant<GameStartedMessage, EliminateResult, WireMessage, NetworkDisconnected>;

class NetworkClient {
public:
    NetworkClient() = default;
    ~NetworkClient();

    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    void connect(const std::string& host, const std::string& service);
    void sendElimination(const EliminateRequest& request);
    void stop() noexcept;
    [[nodiscard]] ThreadSafeMessageQueue<GuiNetworkMessage>& messages() noexcept { return messages_; }

private:
    void receiveLoop() noexcept;
    void handleBody(const std::string& body);

    GameClient client_;
    ThreadSafeMessageQueue<GuiNetworkMessage> messages_;
    std::thread receiver_;
    std::atomic_bool running_{};
};

} // namespace sts
