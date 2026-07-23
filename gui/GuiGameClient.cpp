#include "gui/GuiGameClient.hpp"

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <chrono>
#include <exception>
#include <thread>
#include <type_traits>

namespace sts {

int GuiGameClient::run(const std::string& host, const std::string& service) {
    try {
        network_.connect(host, service);
        state_.phase = ClientConnectionPhase::Connected;
        state_.status = "已连接，等待服务器分配玩家编号";
    } catch (const std::exception& exception) {
        state_.phase = ClientConnectionPhase::Disconnected;
        state_.status = "无法连接服务器";
        state_.lastError = exception.what();
    }

    while (window_.isOpen()) {
        handleEvents();
        consumeNetworkMessages();
        const auto size = window_.renderWindow().getSize();
        const BoardGeometry geometry = BoardGeometry::centered(static_cast<float>(size.x), static_cast<float>(size.y));
        renderer_.draw(window_.renderWindow(), state_, input_, geometry);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    network_.stop();
    return 0;
}

void GuiGameClient::handleEvents() {
    while (const auto event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
            continue;
        }
        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Escape) {
                input_.clear();
            }
        }
        const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>();
        if (mouse == nullptr) {
            continue;
        }
        if (mouse->button == sf::Mouse::Button::Right) {
            input_.clear();
            continue;
        }
        if (mouse->button != sf::Mouse::Button::Left) {
            continue;
        }

        const sf::Vector2f point{static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)};
        if (renderer_.clearButtonBounds(window_.renderWindow()).contains(point)) {
            input_.clear();
            continue;
        }
        if (renderer_.confirmButtonBounds(window_.renderWindow()).contains(point)) {
            if (input_.canConfirm(state_)) {
                const EliminateRequest request = input_.makeRequest(state_);
                state_.requestPending = true;
                state_.status = "操作处理中";
                network_.sendElimination(request);
            }
            continue;
        }

        const auto size = window_.renderWindow().getSize();
        const BoardGeometry geometry = BoardGeometry::centered(static_cast<float>(size.x), static_cast<float>(size.y));
        if (const auto display = geometry.displayPositionAt(point.x, point.y); display.has_value()) {
            static_cast<void>(input_.trySelect(*display, state_));
        }
    }
}

void GuiGameClient::consumeNetworkMessages() {
    while (const auto message = network_.messages().tryPop()) {
        handleMessage(*message);
    }
}

void GuiGameClient::handleMessage(const GuiNetworkMessage& message) {
    std::visit([this](const auto& value) {
        using Value = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Value, GameStartedMessage>) {
            state_.apply(value);
            input_.clear();
        } else if constexpr (std::is_same_v<Value, EliminateResult>) {
            state_.apply(value);
            input_.applyServerResult(value);
        } else if constexpr (std::is_same_v<Value, WireMessage>) {
            handleWireMessage(value);
        } else if constexpr (std::is_same_v<Value, NetworkDisconnected>) {
            state_.phase = ClientConnectionPhase::Disconnected;
            state_.requestPending = false;
            state_.status = "服务器断开";
            state_.lastError = value.error;
            input_.clear();
        }
    }, message);
}

void GuiGameClient::handleWireMessage(const WireMessage& message) {
    switch (message.type) {
    case MessageType::JoinRoom:
        try {
            state_.setPlayerId(std::stoi(message.payload));
            state_.status = "你的玩家编号：" + std::to_string(state_.playerId);
        } catch (const std::exception&) {
            state_.phase = ClientConnectionPhase::Disconnected;
            state_.setError("服务器发送了无效的玩家编号");
        }
        break;
    case MessageType::PlayerReady:
        state_.phase = ClientConnectionPhase::WaitingForOpponent;
        state_.status = "等待另一名玩家";
        break;
    case MessageType::OpponentDisconnected:
        state_.phase = ClientConnectionPhase::GameOver;
        state_.requestPending = false;
        state_.status = "对手断开连接";
        input_.clear();
        break;
    case MessageType::GameEnded:
        state_.phase = ClientConnectionPhase::GameOver;
        state_.requestPending = false;
        state_.status = message.payload.find("opponent disconnected") != std::string::npos
                            ? "游戏结束：对手断开连接"
                            : "游戏结束";
        state_.lastError = message.payload;
        input_.clear();
        break;
    case MessageType::Error:
        state_.requestPending = false;
        if (message.payload == "RoomFull") {
            state_.phase = ClientConnectionPhase::GameOver;
            state_.status = "房间已满";
        } else {
            state_.status = "服务器拒绝了操作";
        }
        state_.lastError = message.payload;
        break;
    default:
        break;
    }
}

} // namespace sts
