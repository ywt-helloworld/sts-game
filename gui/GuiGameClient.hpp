#pragma once

#include "gui/BoardRenderer.hpp"
#include "gui/ClientGameState.hpp"
#include "gui/GameWindow.hpp"
#include "gui/InputController.hpp"
#include "gui/NetworkClient.hpp"

#include <string>

namespace sts {

class GuiGameClient {
public:
    int run(const std::string& host, const std::string& service);

private:
    void handleEvents();
    void consumeNetworkMessages();
    void handleMessage(const GuiNetworkMessage& message);
    void handleWireMessage(const WireMessage& message);

    GameWindow window_;
    BoardRenderer renderer_;
    InputController input_;
    ClientGameState state_;
    NetworkClient network_;
};

} // namespace sts
