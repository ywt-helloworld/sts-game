#include "gui/GuiGameClient.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: sts_gui_client <server_ip> <port>\n";
        return 2;
    }
    try {
        sts::GuiGameClient client;
        return client.run(argv[1], argv[2]);
    } catch (const std::exception& exception) {
        std::cerr << "GUI client error: " << exception.what() << '\n';
        return 1;
    }
}
