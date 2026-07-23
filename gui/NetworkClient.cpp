#include "gui/NetworkClient.hpp"

#include <exception>

namespace sts {

NetworkClient::~NetworkClient() {
    stop();
}

void NetworkClient::connect(const std::string& host, const std::string& service) {
    if (running_) {
        return;
    }
    client_.connect(host, service);
    running_ = true;
    receiver_ = std::thread(&NetworkClient::receiveLoop, this);
}

void NetworkClient::sendElimination(const EliminateRequest& request) {
    try {
        client_.sendElimination(request);
    } catch (const std::exception& exception) {
        messages_.push(NetworkDisconnected{exception.what()});
        running_ = false;
        client_.close();
    }
}

void NetworkClient::stop() noexcept {
    const bool wasRunning = running_.exchange(false);
    if (wasRunning) {
        client_.close();
    }
    if (receiver_.joinable()) {
        receiver_.join();
    }
}

void NetworkClient::receiveLoop() noexcept {
    try {
        while (running_) {
            handleBody(client_.receiveOne());
        }
    } catch (const std::exception& exception) {
        if (running_.exchange(false)) {
            messages_.push(NetworkDisconnected{exception.what()});
        }
    }
}

void NetworkClient::handleBody(const std::string& body) {
    if (const auto started = deserializeGameStartedMessage(body); started.has_value()) {
        messages_.push(*started);
    } else if (const auto result = deserializeEliminateResult(body); result.has_value()) {
        messages_.push(*result);
    } else if (const auto wire = deserializeWireMessage(body); wire.has_value()) {
        messages_.push(*wire);
    } else {
        messages_.push(NetworkDisconnected{"received an invalid server message"});
    }
}

} // namespace sts
