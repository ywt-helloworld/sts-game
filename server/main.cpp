#include "server/GameServer.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    try {
        const auto port = static_cast<std::uint16_t>(argc > 1 ? std::stoul(argv[1]) : 4242U);
        sts::GameServer server(port);
        server.run();
    } catch (const std::exception& exception) {
        std::cerr << "Server error: " << exception.what() << '\n';
        return 1;
    }
}
