#include "common/Protocol.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

namespace sts {
namespace {

template <typename T>
void writeSnapshot(std::ostream& output, const T& board) {
    for (const auto& row : board) {
        for (const auto& piece : row) {
            output << static_cast<int>(piece.type) << ' '
                   << static_cast<int>(piece.color) << ' '
                   << piece.position.row << ' ' << piece.position.column << ' '
                   << piece.attributeValue << ' '
                   << piece.heroId << ' ' << static_cast<int>(piece.heroType) << ' '
                   << piece.currentHp << ' ' << piece.maxHp << ' '
                   << piece.currentBaseAttackDamage << ' ' << piece.defense << ' '
                   << piece.shield << ' ' << piece.vulnerableLayers << ' ' << piece.weakLayers << ' '
                   << piece.radiantStars << ' ' << piece.lightningOrbs << ' ' << piece.alive << ' ';
        }
    }
}

bool readSnapshot(std::istream& input, BoardSnapshot& board) {
    for (auto& row : board) {
        for (auto& piece : row) {
            int type{};
            int color{};
            int heroType{};
            if (!(input >> type >> color >> piece.position.row >> piece.position.column >> piece.attributeValue >>
                  piece.heroId >> heroType >> piece.currentHp >> piece.maxHp >>
                  piece.currentBaseAttackDamage >> piece.defense >> piece.shield >>
                  piece.vulnerableLayers >> piece.weakLayers >> piece.radiantStars >>
                  piece.lightningOrbs >> piece.alive) ||
                type < static_cast<int>(PieceType::Box) || type > static_cast<int>(PieceType::Hero) ||
                color < static_cast<int>(PieceColor::Red) || color > static_cast<int>(PieceColor::Blue) ||
                heroType < static_cast<int>(HeroType::None) ||
                heroType > static_cast<int>(HeroType::ChickenPot)) {
                return false;
            }
            piece.type = static_cast<PieceType>(type);
            piece.color = static_cast<PieceColor>(color);
            piece.heroType = static_cast<HeroType>(heroType);
        }
    }
    return true;
}

void writeGameSnapshot(std::ostream& output, const GameSnapshot& game) {
    output << static_cast<int>(game.phase) << ' ' << game.currentPlayerId << ' ' << game.turnId << ' '
           << game.winnerPlayerId.value_or(-1) << ' ';
    for (const TowerSnapshot& tower : game.towers) {
        output << tower.playerId << ' ' << tower.currentHp << ' ' << tower.maxHp << ' '
               << tower.vulnerableLayers << ' ' << tower.weakLayers << ' '
               << tower.destroyed << ' ';
    }
    writeSnapshot(output, game.board);
    output << game.openingTurnPending << ' ';
}

bool readGameSnapshot(std::istream& input, GameSnapshot& game) {
    int phase{};
    int winner{};
    if (!(input >> phase >> game.currentPlayerId >> game.turnId >> winner) ||
        phase < static_cast<int>(GamePhase::WaitingForPlayers) ||
        phase > static_cast<int>(GamePhase::Finished)) {
        return false;
    }
    game.phase = static_cast<GamePhase>(phase);
    game.winnerPlayerId = winner >= 0 ? std::optional<int>{winner} : std::nullopt;
    for (TowerSnapshot& tower : game.towers) {
        if (!(input >> tower.playerId >> tower.currentHp >> tower.maxHp >>
              tower.vulnerableLayers >> tower.weakLayers >> tower.destroyed)) {
            return false;
        }
    }
    return readSnapshot(input, game.board) && static_cast<bool>(input >> game.openingTurnPending);
}

void writeCombatEvent(std::ostream& output, const CombatEvent& event) {
    output << static_cast<int>(event.type) << ' '
           << event.attackerHeroId.value_or(0) << ' '
           << event.targetHeroId.value_or(0) << ' '
           << event.actingPlayerId.value_or(-1) << ' '
           << event.targetPlayerId.value_or(-1) << ' '
           << event.damage << ' ' << event.remainingHp << ' '
           << event.winnerPlayerId.value_or(-1) << ' '
           << static_cast<int>(event.damageKind) << ' '
           << event.baseDamage << ' ' << event.damageAfterWeak << ' '
           << event.damageAfterVulnerable << ' ' << event.damageAfterDefense << ' '
           << event.shieldAbsorbed << ' ' << event.remainingShield << ' '
           << event.amount << ' ' << event.vulnerableLayers << ' ' << event.weakLayers << ' '
           << event.radiantStars << ' ' << event.lightningOrbs << ' '
           << event.previousLayers << ' ' << event.addedLayers << ' ' << event.totalLayers << ' ';
    output << event.calculatedDamage << ' ' << event.hpDamageApplied << ' '
           << event.targetRemainingHp << ' '
           << static_cast<int>(event.towerDamageSource) << ' '
           << event.shieldBefore << ' ' << event.shieldAfter << ' '
           << event.hpBefore << ' ' << event.hpAfter << ' '
           << event.overflowDamage << ' ' << event.towerHpBefore << ' '
           << event.towerDamageApplied << ' ' << event.towerHpAfter << ' '
           << event.towerDestroyed << ' '
           << static_cast<int>(event.targetType) << ' '
           << event.targetTowerPlayerId.value_or(-1) << ' '
           << event.redirectedBecauseHeroDied << ' ';
}

bool readCombatEvent(std::istream& input, CombatEvent& event) {
    int type{};
    HeroId attacker{};
    HeroId target{};
    int actingPlayer{};
    int targetPlayer{};
    int winner{};
    int damageKind{};
    int towerDamageSource{};
    int targetType{};
    int targetTowerPlayerId{};
    if (!(input >> type >> attacker >> target >> actingPlayer >> targetPlayer >>
          event.damage >> event.remainingHp >> winner >> damageKind >>
          event.baseDamage >> event.damageAfterWeak >> event.damageAfterVulnerable >>
          event.damageAfterDefense >> event.shieldAbsorbed >> event.remainingShield >>
          event.amount >> event.vulnerableLayers >> event.weakLayers >>
          event.radiantStars >> event.lightningOrbs >> event.previousLayers >>
          event.addedLayers >> event.totalLayers >> event.calculatedDamage >>
          event.hpDamageApplied >> event.targetRemainingHp >>
          towerDamageSource >> event.shieldBefore >> event.shieldAfter >>
          event.hpBefore >> event.hpAfter >> event.overflowDamage >>
          event.towerHpBefore >> event.towerDamageApplied >> event.towerHpAfter >>
          event.towerDestroyed >> targetType >> targetTowerPlayerId >>
          event.redirectedBecauseHeroDied) ||
        type < static_cast<int>(CombatEventType::OpeningTurnCompleted) ||
        type > static_cast<int>(CombatEventType::GameFinished) ||
        damageKind < static_cast<int>(DamageKind::NormalAttack) ||
        damageKind > static_cast<int>(DamageKind::Lightning) ||
        towerDamageSource < static_cast<int>(TowerDamageSource::DirectAttack) ||
        towerDamageSource > static_cast<int>(TowerDamageSource::Overflow) ||
        targetType < static_cast<int>(CombatTargetType::Hero) ||
        targetType > static_cast<int>(CombatTargetType::Tower)) {
        return false;
    }
    event.type = static_cast<CombatEventType>(type);
    event.damageKind = static_cast<DamageKind>(damageKind);
    event.towerDamageSource = static_cast<TowerDamageSource>(towerDamageSource);
    event.targetType = static_cast<CombatTargetType>(targetType);
    event.targetTowerPlayerId =
        targetTowerPlayerId < 0 ? std::nullopt : std::optional<int>{targetTowerPlayerId};
    event.attackerHeroId = attacker == 0 ? std::nullopt : std::optional<HeroId>{attacker};
    event.targetHeroId = target == 0 ? std::nullopt : std::optional<HeroId>{target};
    event.actingPlayerId = actingPlayer < 0 ? std::nullopt : std::optional<int>{actingPlayer};
    event.targetPlayerId = targetPlayer < 0 ? std::nullopt : std::optional<int>{targetPlayer};
    event.winnerPlayerId = winner < 0 ? std::nullopt : std::optional<int>{winner};
    return true;
}

} // namespace

std::string serialize(const EliminateRequest& request) {
    std::ostringstream output;
    output << static_cast<int>(MessageType::EliminateRequest) << ' ' << request.playerId << ' '
           << request.turnId << ' ' << request.path.size();
    for (const Position position : request.path) {
        output << ' ' << position.row << ' ' << position.column;
    }
    return output.str();
}

std::optional<EliminateRequest> deserializeEliminateRequest(const std::string& body) {
    std::istringstream input(body);
    int type{};
    EliminateRequest request;
    std::size_t count{};
    if (!(input >> type >> request.playerId >> request.turnId >> count) ||
        type != static_cast<int>(MessageType::EliminateRequest) || count > 50U) {
        return std::nullopt;
    }
    request.path.resize(count);
    for (Position& position : request.path) {
        if (!(input >> position.row >> position.column)) {
            return std::nullopt;
        }
    }
    return request;
}

std::string serialize(const EliminateResult& result) {
    std::ostringstream output;
    output << static_cast<int>(MessageType::EliminateResult) << ' ' << result.success << ' '
           << std::quoted(result.errorMessage) << ' ';
    writeGameSnapshot(output, result.game);
    output << result.combatEvents.size() << ' ';
    for (const CombatEvent& event : result.combatEvents) {
        writeCombatEvent(output, event);
    }
    return output.str();
}

std::optional<EliminateResult> deserializeEliminateResult(const std::string& body) {
    std::istringstream input(body);
    int type{};
    EliminateResult result;
    std::size_t eventCount{};
    if (!(input >> type >> result.success >> std::quoted(result.errorMessage)) ||
        type != static_cast<int>(MessageType::EliminateResult) || !readGameSnapshot(input, result.game) ||
        !(input >> eventCount) || eventCount > 4096U) {
        return std::nullopt;
    }
    result.combatEvents.resize(eventCount);
    for (CombatEvent& event : result.combatEvents) {
        if (!readCombatEvent(input, event)) {
            return std::nullopt;
        }
    }
    return result;
}

std::string serialize(const GameStartedMessage& message) {
    std::ostringstream output;
    output << static_cast<int>(MessageType::GameStarted) << ' ';
    writeGameSnapshot(output, message.game);
    return output.str();
}

std::optional<GameStartedMessage> deserializeGameStartedMessage(const std::string& body) {
    std::istringstream input(body);
    int type{};
    GameStartedMessage message;
    if (!(input >> type) || type != static_cast<int>(MessageType::GameStarted) ||
        !readGameSnapshot(input, message.game)) {
        return std::nullopt;
    }
    return message;
}

std::string serializeWireMessage(WireMessage message) {
    std::ostringstream output;
    output << static_cast<int>(message.type) << ' ' << std::quoted(message.payload);
    return output.str();
}

std::optional<WireMessage> deserializeWireMessage(const std::string& body) {
    std::istringstream input(body);
    int type{};
    WireMessage message;
    if (!(input >> type >> std::quoted(message.payload)) || type < 0 || type > static_cast<int>(MessageType::Error)) {
        return std::nullopt;
    }
    message.type = static_cast<MessageType>(type);
    return message;
}

std::vector<std::byte> makeLengthPrefixedFrame(const std::string& body) {
    if (body.size() > std::numeric_limits<std::uint32_t>::max()) {
        return {};
    }
    const auto length = static_cast<std::uint32_t>(body.size());
    std::vector<std::byte> frame(4U + body.size());
    frame[0] = static_cast<std::byte>((length >> 24U) & 0xffU);
    frame[1] = static_cast<std::byte>((length >> 16U) & 0xffU);
    frame[2] = static_cast<std::byte>((length >> 8U) & 0xffU);
    frame[3] = static_cast<std::byte>(length & 0xffU);
    for (std::size_t index = 0; index < body.size(); ++index) {
        frame[index + 4U] = static_cast<std::byte>(static_cast<unsigned char>(body[index]));
    }
    return frame;
}

std::optional<std::uint32_t> decodeFrameLength(const std::array<std::byte, 4>& header) {
    const auto value = (static_cast<std::uint32_t>(std::to_integer<unsigned char>(header[0])) << 24U) |
                       (static_cast<std::uint32_t>(std::to_integer<unsigned char>(header[1])) << 16U) |
                       (static_cast<std::uint32_t>(std::to_integer<unsigned char>(header[2])) << 8U) |
                       static_cast<std::uint32_t>(std::to_integer<unsigned char>(header[3]));
    return value;
}

} // namespace sts
