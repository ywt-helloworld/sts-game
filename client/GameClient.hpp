#pragma once

#include "common/Protocol.hpp"

#include <asio.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace sts {

class GameClient {
public:
    GameClient();
    void connect(const std::string& host, const std::string& service);
    void sendElimination(const EliminateRequest& request);
    [[nodiscard]] std::string receiveOne();
    void close() noexcept;

private:
    using Tcp = asio::ip::tcp;

    asio::io_context ioContext_;
    Tcp::socket socket_;
};

} // namespace sts
