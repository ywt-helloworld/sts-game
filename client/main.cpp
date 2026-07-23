#include "client/GameClient.hpp"
#include "common/PlayerViewTransform.hpp"

#include <condition_variable>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
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
    GamePhase gamePhase{GamePhase::WaitingForPlayers};
    int playerId{-1};
    int currentPlayerId{-1};
    int turnId{};
    bool openingTurnPending{true};
};

std::string heroName(HeroType type) {
    switch (type) {
    case HeroType::IronFighter: return "战士 / IronFighter";
    case HeroType::SilentHunter: return "猎宝 / SilentHunter";
    case HeroType::Regent: return "储君 / Regent";
    case HeroType::ChickenPot: return "鸡煲 / ChickenPot";
    case HeroType::None: return "无";
    }
    return "未知";
}

std::string combatEventText(const CombatEvent& event) {
    const std::string attacker = event.attackerHeroId ? "H" + std::to_string(*event.attackerHeroId) : "";
    const std::string target = event.targetHeroId ? "H" + std::to_string(*event.targetHeroId) : "高塔";
    switch (event.type) {
    case CombatEventType::OpeningTurnCompleted: return "开局布阵完成，本回合未触发战斗";
    case CombatEventType::CombatStarted: return "战斗开始";
    case CombatEventType::HeroAttacked: return attacker + " 攻击 " + target;
    case CombatEventType::HeroDamaged:
        return target + " 生命伤害 " + std::to_string(event.damage) +
               "，剩余生命 " + std::to_string(event.remainingHp);
    case CombatEventType::VulnerableApplied:
        if (event.targetType == CombatTargetType::Tower &&
            event.redirectedBecauseHeroDied) {
            return attacker + " 击杀目标，" + std::to_string(event.addedLayers) +
                   " 层易伤转移至敌方高塔（共 " +
                   std::to_string(event.totalLayers) + " 层）";
        }
        return target + " 易伤 " + std::to_string(event.previousLayers) + " + " +
               std::to_string(event.addedLayers) + " = " + std::to_string(event.totalLayers);
    case CombatEventType::VulnerableReduced: return target + " 易伤降为 " + std::to_string(event.vulnerableLayers);
    case CombatEventType::VulnerableExpired: return target + " 易伤结束";
    case CombatEventType::WeakApplied:
        if (event.targetType == CombatTargetType::Tower &&
            event.redirectedBecauseHeroDied) {
            return attacker + " 击杀目标，" + std::to_string(event.addedLayers) +
                   " 层虚弱转移至敌方高塔（共 " +
                   std::to_string(event.totalLayers) + " 层）";
        }
        return target + " 虚弱 " + std::to_string(event.previousLayers) + " + " +
               std::to_string(event.addedLayers) + " = " + std::to_string(event.totalLayers);
    case CombatEventType::WeakReduced: return target + " 虚弱降为 " + std::to_string(event.weakLayers);
    case CombatEventType::WeakExpired: return target + " 虚弱结束";
    case CombatEventType::ShieldGained: return attacker + " 获得护盾 " + std::to_string(event.amount);
    case CombatEventType::ShieldAbsorbed: return target + " 护盾吸收 " + std::to_string(event.shieldAbsorbed);
    case CombatEventType::ShieldBroken: return target + " 护盾破碎";
    case CombatEventType::HeroHealed: return attacker + " 回复生命 " + std::to_string(event.amount);
    case CombatEventType::RadiantStarsChanged: return attacker + " 辉星变为 " + std::to_string(event.radiantStars);
    case CombatEventType::RegentAttackModeSelected: return attacker + (event.amount ? " 强化攻击" : " 蓄辉攻击");
    case CombatEventType::LightningAttackStarted: return attacker + " 开始闪电攻击";
    case CombatEventType::LightningActivated: return "闪电第 " + std::to_string(event.amount) + " 次激发";
    case CombatEventType::LightningTargetSelected: return "闪电选择 " + target;
    case CombatEventType::LightningOrbChanged: return attacker + " 闪电球变为 " + std::to_string(event.lightningOrbs);
    case CombatEventType::HeroDied: return target + " 阵亡";
    case CombatEventType::OverflowDamageGenerated:
        return "对英雄造成 " + std::to_string(event.calculatedDamage) +
               " 点结算伤害，其中 " + std::to_string(event.overflowDamage) +
               " 点溢出至高塔";
    case CombatEventType::HeroConvertedToBox: return target + " 转回方格";
    case CombatEventType::TowerDamaged:
        return std::string(event.towerDamageSource == TowerDamageSource::Overflow
                               ? "高塔受到溢出伤害 "
                               : "高塔受到直接伤害 ") +
               std::to_string(event.towerDamageApplied);
    case CombatEventType::TowerDestroyed: return "高塔被摧毁";
    case CombatEventType::CombatFinished: return "战斗结算完成";
    case CombatEventType::TurnChanged: return "回合切换";
    case CombatEventType::GameFinished: return "游戏结束";
    }
    return "战斗事件";
}

void print(const std::shared_ptr<ClientState>& state, const std::string& text) {
    std::lock_guard lock(state->outputMutex);
    std::cout << text << std::flush;
}

void printBoard(const std::shared_ptr<ClientState>& state, const GameSnapshot& game) {
    int playerId{};
    {
        std::lock_guard lock(state->mutex);
        playerId = state->playerId;
    }
    const BoardSnapshot& board = game.board;
    std::ostringstream output;
    const TowerSnapshot& ownTower = game.towers[static_cast<std::size_t>(playerId)];
    const TowerSnapshot& enemyTower = game.towers[static_cast<std::size_t>(1 - playerId)];
    const auto printTower = [&output](std::string_view label, const TowerSnapshot& tower) {
        output << label << "：" << tower.currentHp << " / " << tower.maxHp;
        if (tower.vulnerableLayers > 0) {
            output << "  易伤：" << tower.vulnerableLayers;
        }
        if (tower.weakLayers > 0) {
            output << "  虚弱：" << tower.weakLayers;
        }
        output << '\n';
    };
    printTower("敌方高塔", enemyTower);
    printTower("己方高塔", ownTower);
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
    output << "Hero 状态：\n";
    for (int displayRow = 0; displayRow < BoardRows; ++displayRow) {
        for (int displayColumn = 0; displayColumn < BoardColumns; ++displayColumn) {
            const Position logical = PlayerViewTransform::displayToLogical(playerId, {displayRow, displayColumn});
            const PieceSnapshot& piece = board[logical.row][logical.column];
            if (piece.type != PieceType::Hero) {
                continue;
            }
            output << "  (" << displayRow << ',' << displayColumn << ") " << heroName(piece.heroType)
                   << " 属性=" << piece.attributeValue
                   << " 生命=" << piece.currentHp << '/' << piece.maxHp
                   << " 攻击=" << piece.currentBaseAttackDamage
                   << " 护盾=" << piece.shield
                   << " 易伤=" << piece.vulnerableLayers
                   << " 虚弱=" << piece.weakLayers;
            if (piece.heroType == HeroType::Regent) {
                output << " 辉星=" << piece.radiantStars;
            } else if (piece.heroType == HeroType::ChickenPot) {
                output << " 闪电球=" << piece.lightningOrbs;
            }
            output << '\n';
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
    GamePhase gamePhase{};
    bool openingTurnPending{};
    {
        std::lock_guard lock(state->mutex);
        playerId = state->playerId;
        currentPlayerId = state->currentPlayerId;
        turnId = state->turnId;
        gamePhase = state->gamePhase;
        openingTurnPending = state->openingTurnPending;
    }
    std::ostringstream output;
    output << "当前回合：玩家" << currentPlayerId << "，turnId=" << turnId << '\n';
    if (gamePhase == GamePhase::Finished) {
        output << "游戏已经结束。\n";
        print(state, output.str());
        return;
    }
    if (openingTurnPending && playerId == currentPlayerId) {
        output << "开局布阵回合：本回合只生成英雄，不会发动攻击。\n"
               << "请输入下方区域的消除路径，或输入 quit：\n";
    } else if (openingTurnPending) {
        output << "等待先手玩家完成开局布阵（本回合不会发生战斗）。\n";
    } else if (playerId == currentPlayerId) {
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
            state->gamePhase = started->game.phase;
            state->currentPlayerId = started->game.currentPlayerId;
            state->turnId = started->game.turnId;
            state->openingTurnPending = started->game.openingTurnPending;
        }
        print(state, "游戏开始。\n");
        printBoard(state, started->game);
        announceTurn(state);
        return;
    }
    if (const auto result = deserializeEliminateResult(body); result.has_value()) {
        {
            std::lock_guard lock(state->mutex);
            state->gamePhase = result->game.phase;
            state->currentPlayerId = result->game.currentPlayerId;
            state->turnId = result->game.turnId;
            state->openingTurnPending = result->game.openingTurnPending;
        }
        if (result->success) {
            print(state, "操作成功。\n");
        } else {
            print(state, "操作失败：" + result->errorMessage + "\n");
        }
        if (!result->combatEvents.empty()) {
            print(state, "战斗日志（服务器顺序）：\n");
        }
        for (std::size_t index = 0; index < result->combatEvents.size(); ++index) {
            print(state, "  " + std::to_string(index + 1U) + ". " +
                         combatEventText(result->combatEvents[index]) + "\n");
        }
        printBoard(state, result->game);
        announceTurn(state);
        if (result->game.phase == GamePhase::Finished) {
            stop(state);
        }
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
            if (state->gamePhase != GamePhase::Elimination) {
                print(state, "当前不是消除阶段，不能提交操作。\n");
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
