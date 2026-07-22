#include "client/GameClient.hpp"
#include "common/PlayerViewTransform.hpp"

#include <condition_variable>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace sts;

struct ClientState {
    std::mutex mutex;
    std::condition_variable finished;
    std::mutex outputMutex;
    bool running{true};
    bool started{};
    int playerId{-1};
    int currentPlayerId{-1};
    int turnId{};
};

void print(const std::shared_ptr<ClientState>& state, const std::string& text) {
    std::lock_guard lock(state->outputMutex);
    std::cout << text << std::flush;
}

void printBoard(const std::shared_ptr<ClientState>& state, const BoardSnapshot& board) {
    int playerId{};
    {
        std::lock_guard lock(state->mutex);
        playerId = state->playerId;
    }
    std::ostringstream output;
    output << "上方：对方区域\n      ";
    for (int column = 0; column < BoardColumns; ++column) {
        output << column << "  ";
    }
    output << '\n';
    for (int displayRow = 0; displayRow < BoardRows; ++displayRow) {
        output << displayRow << "    ";
        for (int displayColumn = 0; displayColumn < BoardColumns; ++displayColumn) {
            const Position logical = PlayerViewTransform::displayToLogical(playerId, {displayRow, displayColumn});
            const PieceSnapshot& piece = board[logical.row][logical.column];
            const char color = "RYGB"[static_cast<int>(piece.color)];
            output << color << (piece.type == PieceType::Hero ? 'H' : 'B');
            output << ' ';
        }
        output << '\n';
        if (displayRow == PlayerAreaRows - 1) {
            output << "     " << std::string(static_cast<std::size_t>(BoardColumns) * 3U, '-') << "\n"
                   << "下方：你的可消除区域\n";
        }
    }
    print(state, output.str());
}

void stop(const std::shared_ptr<ClientState>& state) {
    {
        std::lock_guard lock(state->mutex);
        state->running = false;
    }
    state->finished.notify_all();
}

bool isRunning(const std::shared_ptr<ClientState>& state) {
    std::lock_guard lock(state->mutex);
    return state->running;
}

std::vector<Position> parseDisplayPath(const std::string& line) {
    std::istringstream input(line);
    std::vector<Position> path;
    std::string token;
    while (input >> token) {
        const std::size_t comma = token.find(',');
        if (comma == std::string::npos || token.find(',', comma + 1U) != std::string::npos) {
            throw std::runtime_error("输入格式应为：0,0 0,1 1,1");
        }
        Position display;
        try {
            display = {std::stoi(token.substr(0, comma)), std::stoi(token.substr(comma + 1U))};
        } catch (const std::exception&) {
            throw std::runtime_error("输入格式应为：0,0 0,1 1,1");
        }
        if (!PlayerViewTransform::isOwnDisplayArea(display)) {
            throw std::runtime_error("只能选择显示棋盘下方第5～9行、列0～4的格子");
        }
        path.push_back(display);
    }
    if (path.empty()) {
        throw std::runtime_error("请输入至少一个坐标，或输入 quit 退出。");
    }
    return path;
}

void announceTurn(const std::shared_ptr<ClientState>& state) {
    int playerId{};
    int currentPlayerId{};
    int turnId{};
    {
        std::lock_guard lock(state->mutex);
        playerId = state->playerId;
        currentPlayerId = state->currentPlayerId;
        turnId = state->turnId;
    }
    std::ostringstream output;
    output << "当前回合：玩家" << currentPlayerId << "，turnId=" << turnId << '\n';
    if (playerId == currentPlayerId) {
        output << "轮到你，请输入下方区域的消除路径（例如 5,0 5,1 6,1），或输入 quit：\n";
    } else {
        output << "等待对手操作（输入 quit 可退出）：\n";
    }
    print(state, output.str());
}

void handleMessage(const std::shared_ptr<ClientState>& state, const std::string& body) {
    if (const auto started = deserializeGameStartedMessage(body); started.has_value()) {
        {
            std::lock_guard lock(state->mutex);
            state->started = true;
            state->currentPlayerId = started->currentPlayerId;
            state->turnId = started->turnId;
        }
        print(state, "游戏开始。\n");
        printBoard(state, started->board);
        announceTurn(state);
        return;
    }
    if (const auto result = deserializeEliminateResult(body); result.has_value()) {
        {
            std::lock_guard lock(state->mutex);
            state->currentPlayerId = result->nextPlayerId;
            state->turnId = result->nextTurnId;
        }
        if (result->success) {
            print(state, "操作成功。\n");
        } else {
            print(state, "操作失败：" + result->errorMessage + "\n");
        }
        printBoard(state, result->board);
        announceTurn(state);
        return;
    }
    const auto message = deserializeWireMessage(body);
    if (!message.has_value()) {
        print(state, "收到无法解析的服务器消息，连接已关闭。\n");
        stop(state);
        return;
    }

    switch (message->type) {
    case MessageType::JoinRoom: {
        try {
            const int playerId = std::stoi(message->payload);
            std::lock_guard lock(state->mutex);
            state->playerId = playerId;
            print(state, "你的玩家编号：" + std::to_string(playerId) + "\n");
        } catch (const std::exception&) {
            print(state, "服务器发送了无效的玩家编号。\n");
            stop(state);
        }
        return;
    }
    case MessageType::PlayerReady:
        print(state, "等待玩家1连接。\n");
        return;
    case MessageType::OpponentDisconnected:
        print(state, "对手已断开连接。\n");
        return;
    case MessageType::GameEnded:
        print(state, "游戏结束：" + message->payload + "\n");
        stop(state);
        return;
    case MessageType::Error:
        print(state, "服务器错误：" + message->payload + "\n");
        return;
    default:
        return;
    }
}

void receiveLoop(const std::shared_ptr<GameClient>& client, const std::shared_ptr<ClientState>& state) {
    try {
        while (isRunning(state)) {
            handleMessage(state, client->receiveOne());
        }
    } catch (const std::exception& exception) {
        if (isRunning(state)) {
            print(state, std::string("服务器已断开或发生网络错误：") + exception.what() + "\n");
            stop(state);
        }
    }
}

void inputLoop(const std::shared_ptr<GameClient>& client, const std::shared_ptr<ClientState>& state) {
    std::string line;
    while (isRunning(state) && std::getline(std::cin, line)) {
        if (line.starts_with("\xEF\xBB\xBF")) {
            line.erase(0, 3);
        }
        if (line == "quit") {
            print(state, "正在退出。\n");
            stop(state);
            client->close();
            return;
        }

        EliminateRequest request;
        try {
            const std::vector<Position> displayPath = parseDisplayPath(line);
            int playerId{};
            {
                std::lock_guard lock(state->mutex);
                playerId = state->playerId;
            }
            for (const Position display : displayPath) {
                request.path.push_back(PlayerViewTransform::displayToLogical(playerId, display));
            }
        } catch (const std::exception& exception) {
            print(state, std::string("输入错误：") + exception.what() + "\n");
            continue;
        }

        {
            std::lock_guard lock(state->mutex);
            if (!state->started) {
                print(state, "尚未收到游戏开始消息，不能提交操作。\n");
                continue;
            }
            if (state->playerId != state->currentPlayerId) {
                print(state, "当前不是你的回合，不能提交操作。\n");
                continue;
            }
            request.playerId = state->playerId;
            request.turnId = state->turnId;
        }

        try {
            client->sendElimination(request);
        } catch (const std::exception& exception) {
            print(state, std::string("发送失败：") + exception.what() + "\n");
            stop(state);
            client->close();
            return;
        }
    }
    if (isRunning(state)) {
        stop(state);
        client->close();
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "用法：sts_client <server_ip> <port>\n";
        return 2;
    }
    try {
        auto client = std::make_shared<GameClient>();
        auto state = std::make_shared<ClientState>();
        client->connect(argv[1], argv[2]);

        std::thread receiver(receiveLoop, client, state);
        std::thread input(inputLoop, client, state);
        {
            std::unique_lock lock(state->mutex);
            state->finished.wait(lock, [&state] { return !state->running; });
        }
        client->close();
        receiver.join();
        input.detach();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "客户端错误：" << exception.what() << '\n';
        return 1;
    }
}
