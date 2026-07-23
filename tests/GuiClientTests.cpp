#include "tests/TestHarness.hpp"

#include "common/PlayerViewTransform.hpp"
#include "gui/BoardGeometry.hpp"
#include "gui/BoxTextureCatalog.hpp"
#include "gui/ClientGameState.hpp"
#include "gui/HeroTextureCatalog.hpp"
#include "gui/InputController.hpp"
#include "gui/ResourceManager.hpp"
#include "gui/StatusEffectDisplay.hpp"
#include "gui/ThreadSafeMessageQueue.hpp"
#include "gui/WindowLayout.hpp"

#include <filesystem>
#include <array>
#include <cmath>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace sts;

bool nearlyEqual(float left, float right) {
    return std::abs(left - right) < 0.001F;
}

BoardSnapshot makeBoard() {
    BoardSnapshot board{};
    for (int row = 0; row < BoardRows; ++row) {
        for (int column = 0; column < BoardColumns; ++column) {
            board[row][column] = {PieceType::Box, PieceColor::Red, {row, column}, 0};
        }
    }
    return board;
}

GameSnapshot makeGameSnapshot(const BoardSnapshot& board,
                              int currentPlayerId,
                              int turnId,
                              GamePhase phase = GamePhase::Elimination) {
    GameSnapshot snapshot{phase,
                          currentPlayerId,
                          turnId,
                          std::nullopt,
                          {TowerSnapshot{0, 1000, 1000}, TowerSnapshot{1, 1000, 1000}},
                          board};
    snapshot.openingTurnPending = false;
    return snapshot;
}

ClientGameState playingState(int playerId = 0) {
    ClientGameState state;
    state.playerId = playerId;
    state.currentPlayerId = playerId;
    state.turnId = 7;
    state.gamePhase = GamePhase::Elimination;
    state.towers = {TowerSnapshot{0, 1000, 1000}, TowerSnapshot{1, 1000, 1000}};
    state.phase = ClientConnectionPhase::Playing;
    state.board = makeBoard();
    return state;
}

void geometryTests() {
    const BoardGeometry initial = BoardGeometry::centered(1600.0F, 1200.0F);
    REQUIRE(nearlyEqual(initial.cellSize, 108.0F));
    REQUIRE(nearlyEqual(initial.left, 530.0F));
    REQUIRE(nearlyEqual(initial.top, 60.0F));
    REQUIRE(initial.displayPositionAt(531.0F, 61.0F) == (Position{0, 0}));
    REQUIRE(initial.displayPositionAt(1069.0F, 1139.0F) == (Position{9, 4}));
    REQUIRE(!initial.displayPositionAt(529.0F, 60.0F).has_value());
    REQUIRE(!initial.displayPositionAt(1070.0F, 1140.0F).has_value());

    const BoardGeometry resized = BoardGeometry::centered(800.0F, 1000.0F);
    REQUIRE(nearlyEqual(resized.cellSize, 88.0F));
    REQUIRE(nearlyEqual(resized.left, 180.0F));
    REQUIRE(nearlyEqual(resized.top, 60.0F));
    REQUIRE(resized.displayPositionAt(619.0F, 939.0F) == (Position{9, 4}));

    const BoardGeometry enlarged = BoardGeometry::centered(1800.0F, 1350.0F);
    REQUIRE(nearlyEqual(enlarged.cellSize, 121.5F));
    REQUIRE(nearlyEqual(enlarged.left, 596.25F));
    REQUIRE(nearlyEqual(enlarged.top, 67.5F));
}

void windowAndButtonLayoutTests() {
    const WindowPlacement standard = fitWindowInsideDesktop(1920U, 1080U);
    REQUIRE(standard.width == 1651U);
    REQUIRE(standard.height == 929U);
    REQUIRE(standard.left == 134);
    REQUIRE(standard.top == 75);
    REQUIRE(standard.width < 1920U);
    REQUIRE(standard.height < 1080U);

    const WindowPlacement large = fitWindowInsideDesktop(2560U, 1440U);
    REQUIRE(large.width == 1800U);
    REQUIRE(large.height == 1100U);
    REQUIRE(large.left == 380);
    REQUIRE(large.top == 170);

    const ButtonLayout clear = clearButtonLayout(1600U, 900U);
    const ButtonLayout confirm = confirmButtonLayout(1600U, 900U);
    REQUIRE(nearlyEqual(clear.left, 24.0F));
    REQUIRE(nearlyEqual(clear.top, 830.0F));
    REQUIRE(nearlyEqual(confirm.left, 1436.0F));
    REQUIRE(nearlyEqual(confirm.top, 830.0F));
    REQUIRE(clear.left >= 0.0F);
    REQUIRE(clear.top + clear.height <= 900.0F);
    REQUIRE(confirm.left + confirm.width <= 1600.0F);
    REQUIRE(confirm.top + confirm.height <= 900.0F);

    const ButtonLayout resized = confirmButtonLayout(900U, 650U);
    REQUIRE(nearlyEqual(resized.left, 736.0F));
    REQUIRE(nearlyEqual(resized.top, 580.0F));
    REQUIRE(resized.left + resized.width <= 900.0F);
    REQUIRE(resized.top + resized.height <= 650.0F);
}

void selectionTests() {
    ClientGameState state = playingState();
    InputController input;
    REQUIRE(!input.trySelect({4, 0}, state));
    REQUIRE(input.trySelect({5, 0}, state));
    REQUIRE(input.trySelect({5, 1}, state));

    (*state.board)[6][2].color = PieceColor::Blue;
    REQUIRE(!input.trySelect({6, 2}, state));
    REQUIRE(!input.trySelect({7, 1}, state));
    REQUIRE(!input.trySelect({5, 0}, state));

    (*state.board)[6][1].type = PieceType::Hero;
    (*state.board)[6][1].color = PieceColor::Red;
    (*state.board)[6][1].attributeValue = 3;
    REQUIRE(input.trySelect({6, 1}, state));
    REQUIRE(input.path().size() == 3U);
    REQUIRE(input.canConfirm(state));
    REQUIRE(input.trySelect({6, 1}, state));
    REQUIRE(input.path().size() == 2U);
    REQUIRE(!input.canConfirm(state));

    state.requestPending = true;
    REQUIRE(!input.trySelect({6, 1}, state));
    REQUIRE(!input.canConfirm(state));
    state.requestPending = false;
    state.currentPlayerId = 1;
    REQUIRE(!input.trySelect({6, 1}, state));
    REQUIRE(!input.canConfirm(state));
}

void pathConversionTests() {
    ClientGameState state = playingState(1);
    InputController input;
    REQUIRE(input.trySelect({5, 0}, state));
    REQUIRE(input.trySelect({5, 1}, state));
    REQUIRE(input.trySelect({6, 1}, state));
    const EliminateRequest request = input.makeRequest(state);
    REQUIRE(request.playerId == 1);
    REQUIRE(request.turnId == 7);
    REQUIRE(request.path == std::vector<Position>({{4, 4}, {4, 3}, {3, 3}}));

    const Position logical{2, 1};
    const Position display0 = PlayerViewTransform::logicalToDisplay(0, logical);
    const Position display1 = PlayerViewTransform::logicalToDisplay(1, logical);
    REQUIRE(PlayerViewTransform::displayToLogical(0, display0) == logical);
    REQUIRE(PlayerViewTransform::displayToLogical(1, display1) == logical);
    REQUIRE((*state.board)[logical.row][logical.column] ==
            (*state.board)[PlayerViewTransform::displayToLogical(1, display1).row]
                          [PlayerViewTransform::displayToLogical(1, display1).column]);
}

void stateAndQueueTests() {
    ClientGameState state;
    state.setPlayerId(0);
    REQUIRE(state.phase == ClientConnectionPhase::Connected);
    const BoardSnapshot board = makeBoard();
    GameSnapshot startedSnapshot = makeGameSnapshot(board, 0, 2);
    startedSnapshot.towers[1].vulnerableLayers = 3;
    startedSnapshot.towers[1].weakLayers = 2;
    state.apply(GameStartedMessage{startedSnapshot});
    REQUIRE(state.phase == ClientConnectionPhase::Playing);
    REQUIRE(state.board == board);
    REQUIRE(state.towers[1].vulnerableLayers == 3);
    REQUIRE(state.towers[1].weakLayers == 2);
    REQUIRE(state.canSelect());
    REQUIRE(state.status == "消除阶段：轮到你");

    ClientGameState opponentState;
    opponentState.setPlayerId(1);
    opponentState.apply(GameStartedMessage{startedSnapshot});
    REQUIRE(opponentState.towers == state.towers);
    REQUIRE(opponentState.towers[1].vulnerableLayers == 3);
    REQUIRE(opponentState.towers[1].weakLayers == 2);

    GameSnapshot openingSnapshot = makeGameSnapshot(board, 0, 1);
    openingSnapshot.openingTurnPending = true;
    state.apply(GameStartedMessage{openingSnapshot});
    REQUIRE(state.openingTurnPending);
    REQUIRE(state.status == "开局布阵回合：本回合只生成英雄，不会发动攻击");
    CombatEvent openingCompleted;
    openingCompleted.type = CombatEventType::OpeningTurnCompleted;
    GameSnapshot afterOpening = makeGameSnapshot(board, 1, 2);
    state.apply(EliminateResult{true, "", afterOpening, {openingCompleted}});
    REQUIRE(!state.openingTurnPending);
    REQUIRE(state.status == "开局布阵完成，等待对方操作");

    BoardSnapshot changed = board;
    changed[5][0].type = PieceType::Hero;
    changed[5][0].attributeValue = 4;
    state.requestPending = true;
    state.apply(EliminateResult{true, "", makeGameSnapshot(changed, 1, 3), {}});
    REQUIRE(state.board == changed);
    REQUIRE(state.currentPlayerId == 1);
    REQUIRE(state.turnId == 3);
    REQUIRE(!state.requestPending);
    REQUIRE(!state.canSelect());

    GameSnapshot finished = makeGameSnapshot(changed, 1, 3, GamePhase::Finished);
    finished.winnerPlayerId = 0;
    finished.towers[1].currentHp = 0;
    state.apply(EliminateResult{true,
                                "",
                                finished,
                                {{CombatEventType::GameFinished, std::nullopt, std::nullopt,
                                  0, 1, 0, 0, 0}}});
    REQUIRE(state.phase == ClientConnectionPhase::GameOver);
    REQUIRE(state.winnerPlayerId == 0);
    REQUIRE(state.towers[1].currentHp == 0);
    REQUIRE(!state.canSelect());

    ThreadSafeMessageQueue<std::string> queue;
    queue.push("message");
    REQUIRE(queue.tryPop() == "message");
    REQUIRE(!queue.tryPop().has_value());
}

void boxTextureTests() {
    REQUIRE(boxTextureFilename(PieceColor::Red, BoxTextureState::Normal) == "normal_red.png");
    REQUIRE(boxTextureFilename(PieceColor::Red, BoxTextureState::Selected) == "selected_red.png");
    REQUIRE(boxTextureFilename(PieceColor::Yellow, BoxTextureState::Normal) == "normal_yellow.png");
    REQUIRE(boxTextureFilename(PieceColor::Yellow, BoxTextureState::Selected) == "selected_yellow.png");
    REQUIRE(boxTextureFilename(PieceColor::Green, BoxTextureState::Normal) == "normal_green.png");
    REQUIRE(boxTextureFilename(PieceColor::Green, BoxTextureState::Selected) == "selected_green.png");
    REQUIRE(boxTextureFilename(PieceColor::Blue, BoxTextureState::Normal) == "normal_blue.png");
    REQUIRE(boxTextureFilename(PieceColor::Blue, BoxTextureState::Selected) == "selected_blue.png");

    const std::array colors{PieceColor::Red, PieceColor::Yellow, PieceColor::Green, PieceColor::Blue};
    const std::array states{BoxTextureState::Normal, BoxTextureState::Selected};
    std::set<std::string_view> filenames;
    for (const PieceColor color : colors) {
        for (const BoxTextureState textureState : states) {
            filenames.insert(boxTextureFilename(color, textureState));
        }
        REQUIRE(boxVisualScale(color, BoxTextureState::Selected) >
                boxVisualScale(color, BoxTextureState::Normal));
        REQUIRE(boxVisualScale(color, BoxTextureState::Selected) <= 1.10F);
    }
    REQUIRE(filenames.size() == 8U);
    REQUIRE(boxVisualScale(PieceColor::Yellow, BoxTextureState::Normal) == 0.99F);
    REQUIRE(boxVisualScale(PieceColor::Red, BoxTextureState::Normal) == 1.0F);
    REQUIRE(pieceUsesBoxTexture(PieceType::Box));
    REQUIRE(!pieceUsesBoxTexture(PieceType::Hero));

    ClientGameState state = playingState();
    InputController input;
    const Position first{5, 0};
    REQUIRE(boxTextureState(input.isSelected(first)) == BoxTextureState::Normal);
    REQUIRE(input.trySelect(first, state));
    REQUIRE(boxTextureState(input.isSelected(first)) == BoxTextureState::Selected);
    REQUIRE(input.undoLast());
    REQUIRE(boxTextureState(input.isSelected(first)) == BoxTextureState::Normal);

    REQUIRE(input.trySelect(first, state));
    REQUIRE(input.trySelect({5, 1}, state));
    REQUIRE(input.trySelect({6, 1}, state));
    input.clear();
    REQUIRE(input.path().empty());
    REQUIRE(boxTextureState(input.isSelected(first)) == BoxTextureState::Normal);

    REQUIRE(!input.trySelect({4, 0}, state));
    REQUIRE(!input.isSelected({4, 0}));
    state.currentPlayerId = 1;
    REQUIRE(!input.trySelect(first, state));
    REQUIRE(!input.isSelected(first));

    state.currentPlayerId = 0;
    REQUIRE(input.trySelect(first, state));
    state.requestPending = true;
    REQUIRE(!input.trySelect({5, 1}, state));
    REQUIRE(input.isSelected(first));

    const BoardSnapshot board = *state.board;
    input.applyServerResult(EliminateResult{false, "rejected", makeGameSnapshot(board, 0, 7), {}});
    REQUIRE(input.isSelected(first));
    input.applyServerResult(EliminateResult{true, "", makeGameSnapshot(board, 1, 8), {}});
    REQUIRE(input.path().empty());

    const std::vector<std::filesystem::path> missingRoots{
        std::filesystem::path{"this-directory-does-not-exist"},
    };
    REQUIRE(!findBoxTextureFile(PieceColor::Red, BoxTextureState::Normal, missingRoots).has_value());
}

void heroTextureTests() {
    REQUIRE(heroTextureFilename(PieceColor::Red) == "hero_red.png");
    REQUIRE(heroTextureFilename(PieceColor::Yellow) == "hero_yellow.png");
    REQUIRE(heroTextureFilename(PieceColor::Blue) == "hero_blue.png");
    REQUIRE(heroTextureFilename(PieceColor::Green) == "hero_green.png");
    REQUIRE(pieceUsesHeroTexture(PieceType::Hero));
    REQUIRE(!pieceUsesHeroTexture(PieceType::Box));

    const std::array colors{PieceColor::Red, PieceColor::Yellow, PieceColor::Green, PieceColor::Blue};
    std::set<std::string_view> filenames;
    for (const PieceColor color : colors) {
        filenames.insert(heroTextureFilename(color));
    }
    REQUIRE(filenames.size() == 4U);

    const HeroVisualLayout normal = fitHeroTextureInsideCell(230U, 335U, 10.0F, 20.0F, 100.0F, 100.0F, false);
    const HeroVisualLayout selected = fitHeroTextureInsideCell(230U, 335U, 10.0F, 20.0F, 100.0F, 100.0F, true);
    REQUIRE(nearlyEqual(normal.width / normal.height, 230.0F / 335.0F));
    REQUIRE(nearlyEqual(normal.left + normal.width / 2.0F, 60.0F));
    REQUIRE(nearlyEqual(normal.top + normal.height / 2.0F, 70.0F));
    REQUIRE(selected.width > normal.width);
    REQUIRE(selected.height > normal.height);
    REQUIRE(nearlyEqual(selected.left + selected.width / 2.0F, 60.0F));
    REQUIRE(nearlyEqual(selected.top + selected.height / 2.0F, 70.0F));
    REQUIRE(nearlyEqual(selected.uniformScale / normal.uniformScale, 1.07F));

    const HeroVisualLayout resized = fitHeroTextureInsideCell(230U, 335U, 0.0F, 0.0F, 200.0F, 200.0F, false);
    REQUIRE(nearlyEqual(resized.width, normal.width * 2.0F));
    REQUIRE(nearlyEqual(resized.height, normal.height * 2.0F));

    ClientGameState state = playingState(0);
    (*state.board)[8][2] = {PieceType::Hero, PieceColor::Blue, {8, 2}, 42};
    const Position initialDisplay = PlayerViewTransform::logicalToDisplay(0, {8, 2});
    REQUIRE(state.pieceAtLogical(PlayerViewTransform::displayToLogical(0, initialDisplay))->type == PieceType::Hero);
    REQUIRE(state.pieceAtLogical({8, 2})->attributeValue == 42);

    (*state.board)[8][2] = {PieceType::Box, PieceColor::Red, {8, 2}, 0};
    (*state.board)[9][2] = {PieceType::Hero, PieceColor::Blue, {9, 2}, 42};
    const Position movedDisplay = PlayerViewTransform::logicalToDisplay(0, {9, 2});
    REQUIRE(movedDisplay != initialDisplay);
    REQUIRE(state.pieceAtLogical(PlayerViewTransform::displayToLogical(0, movedDisplay))->type == PieceType::Hero);

    ClientGameState player1State = playingState(1);
    (*player1State.board)[1][3] = {PieceType::Hero, PieceColor::Green, {1, 3}, 19};
    const Position player1Display = PlayerViewTransform::logicalToDisplay(1, {1, 3});
    REQUIRE(player1Display == (Position{8, 1}));
    REQUIRE(player1State.pieceAtLogical(PlayerViewTransform::displayToLogical(1, player1Display))->color ==
            PieceColor::Green);

    ClientGameState selectionState = playingState(0);
    (*selectionState.board)[5][0] = {PieceType::Hero, PieceColor::Red, {5, 0}, 12};
    InputController input;
    REQUIRE(input.trySelect({5, 0}, selectionState));
    REQUIRE(input.trySelect({5, 1}, selectionState));
    REQUIRE(input.trySelect({6, 1}, selectionState));
    REQUIRE(input.canConfirm(selectionState));
    input.clear();
    REQUIRE(!input.isSelected({5, 0}));

    (*selectionState.board)[5][1].color = PieceColor::Blue;
    REQUIRE(input.trySelect({5, 0}, selectionState));
    REQUIRE(!input.trySelect({5, 1}, selectionState));

    const std::vector<std::filesystem::path> missingRoots{
        std::filesystem::path{"this-directory-does-not-exist"},
    };
    REQUIRE(!findHeroTextureFile(PieceColor::Red, missingRoots).has_value());
}

void statusEffectDisplayTests() {
    REQUIRE(hudTextureFilename(HudIconKind::Vulnerable) == "vulnerable.png");
    REQUIRE(hudTextureFilename(HudIconKind::Weak) == "weak.png");
    REQUIRE(hudTextureFilename(HudIconKind::Shield) == "shield.png");
    REQUIRE(hudTextureFilename(HudIconKind::GloryStar) == "glory_star.png");
    REQUIRE(hudTextureFilename(HudIconKind::LightningChargeOrb) ==
            "lightning_charge_orb.png");
    REQUIRE(hudTextureDirectoryName(HudIconKind::Vulnerable) == "status");
    REQUIRE(hudTextureDirectoryName(HudIconKind::GloryStar) == "resources");

    ResourceManager resources;
    for (const HudIconKind kind :
         {HudIconKind::Vulnerable,
          HudIconKind::Weak,
          HudIconKind::Shield,
          HudIconKind::GloryStar,
          HudIconKind::LightningChargeOrb}) {
        const sf::Texture* texture = resources.hudTextureFor(kind);
        REQUIRE(texture != nullptr);
        REQUIRE(texture->getSize().x > 0U);
        REQUIRE(texture->getSize().y > 0U);
    }

    PieceSnapshot hero;
    hero.type = PieceType::Hero;
    REQUIRE(heroStatusDisplayItems(hero).empty());

    hero.vulnerableLayers = 3;
    REQUIRE(heroStatusDisplayItems(hero) ==
            std::vector<StatusDisplayItem>({{HudIconKind::Vulnerable, 3}}));
    hero.vulnerableLayers = 0;
    hero.weakLayers = 2;
    REQUIRE(heroStatusDisplayItems(hero) ==
            std::vector<StatusDisplayItem>({{HudIconKind::Weak, 2}}));
    hero.weakLayers = 0;
    hero.shield = 64;
    REQUIRE(heroStatusDisplayItems(hero) ==
            std::vector<StatusDisplayItem>({{HudIconKind::Shield, 64}}));

    hero.vulnerableLayers = 3;
    hero.weakLayers = 2;
    const std::vector<StatusDisplayItem> allHeroItems = heroStatusDisplayItems(hero);
    REQUIRE(allHeroItems ==
            std::vector<StatusDisplayItem>({
                {HudIconKind::Vulnerable, 3},
                {HudIconKind::Weak, 2},
                {HudIconKind::Shield, 64},
            }));

    TowerSnapshot tower{1, 872, 1000};
    REQUIRE(towerStatusDisplayItems(tower).empty());
    tower.vulnerableLayers = 2;
    tower.weakLayers = 1;
    REQUIRE(towerStatusDisplayItems(tower) ==
            std::vector<StatusDisplayItem>({
                {HudIconKind::Vulnerable, 2},
                {HudIconKind::Weak, 1},
            }));
    tower.vulnerableLayers = 0;
    tower.weakLayers = 0;
    REQUIRE(towerStatusDisplayItems(tower).empty());

    REQUIRE(nearlyEqual(statusIconSize(40.0F), 16.0F));
    REQUIRE(nearlyEqual(statusIconSize(108.0F), 19.44F));
    REQUIRE(nearlyEqual(statusIconSize(200.0F), 24.0F));
    const StatusStripMetrics metrics = statusStripMetrics(3U, 108.0F);
    REQUIRE(metrics.iconSize >= 16.0F);
    REQUIRE(metrics.iconSize <= 24.0F);
    REQUIRE(metrics.totalWidth < 108.0F);

    BoardSnapshot synchronizedBoard = makeBoard();
    synchronizedBoard[5][0].type = PieceType::Hero;
    synchronizedBoard[5][0].vulnerableLayers = 5;
    synchronizedBoard[5][0].weakLayers = 4;
    synchronizedBoard[5][0].shield = 32;
    GameSnapshot synchronizedSnapshot = makeGameSnapshot(synchronizedBoard, 0, 9);
    synchronizedSnapshot.towers[1] = tower;
    synchronizedSnapshot.towers[1].vulnerableLayers = 5;
    synchronizedSnapshot.towers[1].weakLayers = 4;

    ClientGameState player0;
    player0.setPlayerId(0);
    player0.apply(GameStartedMessage{synchronizedSnapshot});
    ClientGameState player1;
    player1.setPlayerId(1);
    player1.apply(GameStartedMessage{synchronizedSnapshot});
    REQUIRE(heroStatusDisplayItems((*player0.board)[5][0]) ==
            heroStatusDisplayItems((*player1.board)[5][0]));
    REQUIRE(towerStatusDisplayItems(player0.towers[1]) ==
            towerStatusDisplayItems(player1.towers[1]));

    PieceSnapshot ironFighter;
    ironFighter.type = PieceType::Hero;
    ironFighter.heroType = HeroType::IronFighter;
    REQUIRE(heroResourceDisplayItems(ironFighter).empty());

    PieceSnapshot silentHunter = ironFighter;
    silentHunter.heroType = HeroType::SilentHunter;
    REQUIRE(heroResourceDisplayItems(silentHunter).empty());

    PieceSnapshot regent = ironFighter;
    regent.heroType = HeroType::Regent;
    regent.radiantStars = 4;
    REQUIRE(heroResourceDisplayItems(regent) ==
            std::vector<StatusDisplayItem>({{HudIconKind::GloryStar, 4}}));
    regent.radiantStars = 1;
    REQUIRE(heroResourceDisplayItems(regent).front().value == 1);

    PieceSnapshot chickenPot = ironFighter;
    chickenPot.heroType = HeroType::ChickenPot;
    chickenPot.lightningOrbs = 0;
    REQUIRE(heroResourceDisplayItems(chickenPot) ==
            std::vector<StatusDisplayItem>(
                {{HudIconKind::LightningChargeOrb, 0}}));
    chickenPot.lightningOrbs = 1;
    REQUIRE(heroResourceDisplayItems(chickenPot).front().value == 1);

    synchronizedBoard[5][1] = regent;
    synchronizedBoard[5][1].position = {5, 1};
    synchronizedBoard[5][2] = chickenPot;
    synchronizedBoard[5][2].position = {5, 2};
    synchronizedSnapshot = makeGameSnapshot(synchronizedBoard, 0, 10);
    player0.apply(GameStartedMessage{synchronizedSnapshot});
    player1.apply(GameStartedMessage{synchronizedSnapshot});
    REQUIRE(heroResourceDisplayItems((*player0.board)[5][1]) ==
            heroResourceDisplayItems((*player1.board)[5][1]));
    REQUIRE(heroResourceDisplayItems((*player0.board)[5][2]) ==
            heroResourceDisplayItems((*player1.board)[5][2]));
}

} // namespace

void runGuiClientTests() {
    geometryTests();
    windowAndButtonLayoutTests();
    selectionTests();
    pathConversionTests();
    stateAndQueueTests();
    boxTextureTests();
    heroTextureTests();
    statusEffectDisplayTests();
}
