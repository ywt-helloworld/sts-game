#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

#include "common/Protocol.hpp"
#include "game/Hero.hpp"
#include "game/HeroFactory.hpp"
#include "game/Tower.hpp"

using namespace sts;

void runProtocolTests() {
    const EliminateRequest request{1, 9, {{4, 0}, {4, 1}, {3, 1}}};
    const auto decodedRequest = deserializeEliminateRequest(serialize(request));
    REQUIRE(decodedRequest.has_value());
    REQUIRE(decodedRequest->playerId == 1);
    REQUIRE(decodedRequest->turnId == 9);
    REQUIRE(decodedRequest->path == request.path);

    const auto frame = makeLengthPrefixedFrame("hello");
    REQUIRE(frame.size() == 9U);
    const std::array<std::byte, 4> header{frame[0], frame[1], frame[2], frame[3]};
    REQUIRE(decodeFrameLength(header).value() == 5U);

    BoxBoard board = test::solidBoard();
    board.setPieceAt({2, 3}, HeroFactory::create(77, PieceColor::Green, {2, 3}, 47, 1));
    Hero* hunter = board.heroById(77);
    REQUIRE(hunter != nullptr);
    REQUIRE(hunter->gainShield(19) == 19);
    hunter->addWeakLayers(2);
    hunter->addVulnerableLayers(1);
    board.setPieceAt({1, 2}, HeroFactory::create(78, PieceColor::Yellow, {1, 2}, 3, 1));
    board.setPieceAt({1, 3}, HeroFactory::create(79, PieceColor::Blue, {1, 3}, 3, 1));

    GameSnapshot snapshot{GamePhase::Elimination,
                          0,
                          10,
                          std::nullopt,
                          {TowerSnapshot{0, 900, Tower::MaxHp}, TowerSnapshot{1, 750, Tower::MaxHp}},
                          board.snapshot()};
    snapshot.openingTurnPending = false;

    CombatEvent started;
    started.type = CombatEventType::CombatStarted;
    started.actingPlayerId = 0;
    started.targetPlayerId = 1;

    CombatEvent damaged;
    damaged.type = CombatEventType::HeroDamaged;
    damaged.attackerHeroId = 77;
    damaged.targetHeroId = 88;
    damaged.actingPlayerId = 0;
    damaged.targetPlayerId = 1;
    damaged.damage = 7;
    damaged.remainingHp = 0;
    damaged.damageKind = DamageKind::Lightning;
    damaged.baseDamage = 12;
    damaged.damageAfterWeak = 12;
    damaged.damageAfterVulnerable = 18;
    damaged.damageAfterDefense = 18;
    damaged.shieldAbsorbed = 5;
    damaged.remainingShield = 0;
    damaged.calculatedDamage = 18;
    damaged.hpDamageApplied = 7;
    damaged.overkillDamage = 6;
    damaged.targetRemainingHp = 0;

    CombatEvent status;
    status.type = CombatEventType::WeakApplied;
    status.attackerHeroId = 77;
    status.targetHeroId = 88;
    status.actingPlayerId = 0;
    status.targetPlayerId = 1;
    status.amount = 2;
    status.weakLayers = 2;
    status.previousLayers = 1;
    status.addedLayers = 2;
    status.totalLayers = 3;

    CombatEvent vulnerable;
    vulnerable.type = CombatEventType::VulnerableApplied;
    vulnerable.attackerHeroId = 77;
    vulnerable.targetHeroId = 88;
    vulnerable.actingPlayerId = 0;
    vulnerable.targetPlayerId = 1;
    vulnerable.amount = 1;
    vulnerable.vulnerableLayers = 3;
    vulnerable.previousLayers = 2;
    vulnerable.addedLayers = 1;
    vulnerable.totalLayers = 3;

    CombatEvent stars;
    stars.type = CombatEventType::RadiantStarsChanged;
    stars.attackerHeroId = 78;
    stars.amount = -2;
    stars.radiantStars = 1;

    CombatEvent orbs;
    orbs.type = CombatEventType::LightningOrbChanged;
    orbs.attackerHeroId = 79;
    orbs.amount = -1;
    orbs.lightningOrbs = 0;

    const std::vector<CombatEvent> events{started, damaged, status, vulnerable, stars, orbs};
    const EliminateResult result{true, "", snapshot, events};
    const auto decodedResult = deserializeEliminateResult(serialize(result));
    REQUIRE(decodedResult.has_value());
    REQUIRE(decodedResult->success);
    REQUIRE(decodedResult->game == snapshot);
    REQUIRE(!decodedResult->game.openingTurnPending);
    REQUIRE(decodedResult->combatEvents == events);

    const PieceSnapshot& hero = decodedResult->game.board[2][3];
    REQUIRE(hero.type == PieceType::Hero);
    REQUIRE(hero.heroId == 77);
    REQUIRE(hero.heroType == HeroType::SilentHunter);
    REQUIRE(hero.color == PieceColor::Green);
    REQUIRE(hero.position == (Position{2, 3}));
    REQUIRE(hero.attributeValue == 47);
    REQUIRE(hero.currentHp == 658);
    REQUIRE(hero.maxHp == 658);
    REQUIRE(hero.currentBaseAttackDamage == 564);
    REQUIRE(hero.defense == 0);
    REQUIRE(hero.shield == 19);
    REQUIRE(hero.vulnerableLayers == 1);
    REQUIRE(hero.weakLayers == 2);
    REQUIRE(hero.radiantStars == 0);
    REQUIRE(hero.lightningOrbs == 0);

    REQUIRE(decodedResult->game.board[1][2].radiantStars == Regent::InitialRadiantStars);
    REQUIRE(decodedResult->game.board[1][2].lightningOrbs == 0);
    REQUIRE(decodedResult->game.board[1][3].radiantStars == 0);
    REQUIRE(decodedResult->game.board[1][3].lightningOrbs == ChickenPot::InitialLightningOrbs);

    const GameStartedMessage gameStarted{snapshot};
    const auto decodedStarted = deserializeGameStartedMessage(serialize(gameStarted));
    REQUIRE(decodedStarted.has_value());
    REQUIRE(decodedStarted->game == snapshot);

    snapshot.phase = GamePhase::Finished;
    snapshot.winnerPlayerId = 1;
    const auto decodedFinished = deserializeGameStartedMessage(serialize(GameStartedMessage{snapshot}));
    REQUIRE(decodedFinished.has_value());
    REQUIRE(decodedFinished->game.phase == GamePhase::Finished);
    REQUIRE(decodedFinished->game.winnerPlayerId == 1);
}
