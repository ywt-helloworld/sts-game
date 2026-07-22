#include "client/GameClient.hpp"

#include <stdexcept>

namespace sts {

GameClient::GameClient() : socket_(ioContext_) {}

void GameClient::connect(const std::string& host, const std::string& service) {
    Tcp::resolver resolver(ioContext_);
    const auto endpoints = resolver.resolve(host, service);
    asio::connect(socket_, endpoints);
}

void GameClient::sendElimination(const EliminateRequest& request) {
    const std::vector<std::byte> frame = makeLengthPrefixedFrame(serialize(request));
    asio::write(socket_, asio::buffer(frame));
}

std::string GameClient::receiveOne() {
    std::array<std::byte, 4> header{};
    asio::read(socket_, asio::buffer(header));
    const auto length = decodeFrameLength(header);
    if (!length.has_value() || *length > 64U * 1024U) {
        throw std::runtime_error("server sent an invalid message length");
    }
    std::string body(*length, '\0');
    asio::read(socket_, asio::buffer(body));
    return body;
}

void GameClient::close() noexcept {
    std::error_code ignored;
    socket_.shutdown(Tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
}

} // namespace sts
