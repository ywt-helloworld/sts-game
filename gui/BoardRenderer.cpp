#include "gui/BoardRenderer.hpp"

#include "common/PlayerViewTransform.hpp"
#include "gui/BoxTextureCatalog.hpp"
#include "gui/HeroTextureCatalog.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/String.hpp>

#include <algorithm>
#include <sstream>
#include <string>

namespace sts {
namespace {

sf::Color pieceColor(PieceColor color) {
    switch (color) {
    case PieceColor::Red: return {218, 72, 72};
    case PieceColor::Yellow: return {235, 193, 64};
    case PieceColor::Green: return {70, 180, 105};
    case PieceColor::Blue: return {72, 128, 218};
    }
    return sf::Color::White;
}

void drawText(sf::RenderWindow& window,
              const sf::Font* font,
              const std::string& value,
              unsigned size,
              sf::Vector2f position,
              sf::Color color = sf::Color::White) {
    if (font == nullptr) {
        return;
    }
    sf::Text text(*font, sf::String::fromUtf8(value.begin(), value.end()), size);
    text.setPosition(position);
    text.setFillColor(color);
    window.draw(text);
}

sf::Vector2f centerOf(Position display, const BoardGeometry& geometry) {
    return {geometry.left + (static_cast<float>(display.column) + 0.5F) * geometry.cellSize,
            geometry.top + (static_cast<float>(display.row) + 0.5F) * geometry.cellSize};
}

std::string phaseText(GamePhase phase) {
    switch (phase) {
    case GamePhase::WaitingForPlayers: return "等待玩家";
    case GamePhase::Elimination: return "消除阶段";
    case GamePhase::Combat: return "战斗结算中";
    case GamePhase::Finished: return "游戏结束";
    }
    return "未知阶段";
}

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
    const std::string attacker = event.attackerHeroId.has_value()
                                     ? "H" + std::to_string(*event.attackerHeroId)
                                     : "";
    const std::string target = event.targetHeroId.has_value()
                                   ? "H" + std::to_string(*event.targetHeroId)
                                   : "高塔";
    switch (event.type) {
    case CombatEventType::OpeningTurnCompleted: return "开局布阵完成，未触发战斗";
    case CombatEventType::CombatStarted: return "战斗开始";
    case CombatEventType::HeroAttacked:
        return attacker + " 攻击 " + target;
    case CombatEventType::HeroDamaged:
        return target + " 生命伤害 " + std::to_string(event.damage) +
               "，剩余 " + std::to_string(event.remainingHp);
    case CombatEventType::VulnerableApplied:
        if (event.targetType == CombatTargetType::Tower &&
            event.redirectedBecauseHeroDied) {
            return attacker + " 击杀目标，" + std::to_string(event.addedLayers) +
                   " 层易伤转移至敌方高塔（共 " +
                   std::to_string(event.totalLayers) + " 层）";
        }
        return target + " 易伤 " + std::to_string(event.previousLayers) + " + " +
               std::to_string(event.addedLayers) + " = " + std::to_string(event.totalLayers);
    case CombatEventType::VulnerableReduced:
        return target + " 易伤降为 " + std::to_string(event.vulnerableLayers);
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
    case CombatEventType::WeakReduced:
        return target + " 虚弱降为 " + std::to_string(event.weakLayers);
    case CombatEventType::WeakExpired: return target + " 虚弱结束";
    case CombatEventType::ShieldGained:
        return attacker + " 获得护盾 " + std::to_string(event.amount);
    case CombatEventType::ShieldAbsorbed:
        return target + " 护盾吸收 " + std::to_string(event.shieldAbsorbed);
    case CombatEventType::ShieldBroken: return target + " 护盾破碎";
    case CombatEventType::HeroHealed:
        return attacker + " 回复 " + std::to_string(event.amount);
    case CombatEventType::RadiantStarsChanged:
        return attacker + " 辉星变为 " + std::to_string(event.radiantStars);
    case CombatEventType::RegentAttackModeSelected:
        return attacker + (event.amount == 1 ? " 选择强化攻击" : " 选择蓄辉攻击");
    case CombatEventType::LightningAttackStarted: return attacker + " 开始闪电攻击";
    case CombatEventType::LightningActivated:
        return "第 " + std::to_string(event.amount) + " 次闪电激发";
    case CombatEventType::LightningTargetSelected:
        return "闪电选择 " + target;
    case CombatEventType::LightningOrbChanged:
        return attacker + " 闪电球变为 " + std::to_string(event.lightningOrbs);
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

void drawButton(sf::RenderWindow& window,
                const sf::Font* font,
                sf::FloatRect bounds,
                const std::string& label,
                bool enabled) {
    sf::RectangleShape shape(bounds.size);
    shape.setPosition(bounds.position);
    shape.setFillColor(enabled ? sf::Color{55, 142, 92} : sf::Color{72, 76, 84});
    shape.setOutlineColor(sf::Color{185, 190, 200});
    shape.setOutlineThickness(2.0F);
    window.draw(shape);
    drawText(window, font, label, 18U, {bounds.position.x + 14.0F, bounds.position.y + 10.0F},
             enabled ? sf::Color::White : sf::Color{155, 158, 165});
}

void drawTowerBar(sf::RenderWindow& window,
                  const TowerSnapshot& tower,
                  sf::Vector2f position,
                  sf::Color fillColor) {
    constexpr sf::Vector2f size{170.0F, 16.0F};
    sf::RectangleShape background(size);
    background.setPosition(position);
    background.setFillColor(sf::Color{25, 25, 30, 220});
    background.setOutlineColor(sf::Color{220, 205, 175, 190});
    background.setOutlineThickness(1.0F);
    window.draw(background);

    const float ratio = tower.maxHp > 0
                            ? std::clamp(static_cast<float>(tower.currentHp) / static_cast<float>(tower.maxHp), 0.0F, 1.0F)
                            : 0.0F;
    sf::RectangleShape fill({size.x * ratio, size.y});
    fill.setPosition(position);
    fill.setFillColor(fillColor);
    window.draw(fill);
}

} // namespace

BoardRenderer::BoardRenderer()
    : heroRenderer_(resources_), statusRenderer_(resources_) {}

sf::FloatRect BoardRenderer::confirmButtonBounds(const sf::RenderWindow& window) const noexcept {
    const auto size = window.getSize();
    return {{static_cast<float>(size.x) - 164.0F, static_cast<float>(size.y) - 70.0F}, {140.0F, 44.0F}};
}

sf::FloatRect BoardRenderer::clearButtonBounds(const sf::RenderWindow& window) const noexcept {
    const auto size = window.getSize();
    return {{24.0F, static_cast<float>(size.y) - 70.0F}, {140.0F, 44.0F}};
}

void BoardRenderer::draw(sf::RenderWindow& window,
                         const ClientGameState& state,
                         const InputController& input,
                         const BoardGeometry& geometry) {
    window.clear(sf::Color{8, 13, 20});
    const sf::Font* font = resources_.font();

    if (const sf::Texture* background = resources_.background(); background != nullptr) {
        const sf::Vector2u textureSize = background->getSize();
        const sf::Vector2u windowSize = window.getSize();
        const float scale = static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y);
        const float renderedWidth = static_cast<float>(textureSize.x) * scale;
        sf::Sprite sprite(*background);
        sprite.setScale({scale, scale});
        sprite.setPosition({(static_cast<float>(windowSize.x) - renderedWidth) / 2.0F, 0.0F});
        window.draw(sprite);
    }

    constexpr float framePadding = 10.0F;
    sf::RectangleShape boardFrame({geometry.boardWidth() + framePadding * 2.0F,
                                   geometry.boardHeight() + framePadding * 2.0F});
    boardFrame.setPosition({geometry.left - framePadding, geometry.top - framePadding});
    boardFrame.setFillColor(sf::Color{69, 48, 30, 72});
    window.draw(boardFrame);

    const float halfBoardHeight = geometry.cellSize * (static_cast<float>(BoardRows) / 2.0F);
    sf::RectangleShape opponentArea({geometry.boardWidth(), halfBoardHeight});
    opponentArea.setPosition({geometry.left, geometry.top});
    opponentArea.setFillColor(sf::Color{112, 35, 39, 42});
    window.draw(opponentArea);

    sf::RectangleShape ownArea({geometry.boardWidth(), halfBoardHeight});
    ownArea.setPosition({geometry.left, geometry.top + halfBoardHeight});
    ownArea.setFillColor(sf::Color{17, 104, 126, 42});
    window.draw(ownArea);

    const float dividerY = geometry.top + halfBoardHeight;
    sf::RectangleShape dividerShadow({geometry.boardWidth() + 16.0F, 14.0F});
    dividerShadow.setPosition({geometry.left - 8.0F, dividerY - 7.0F});
    dividerShadow.setFillColor(sf::Color{26, 18, 18, 235});
    window.draw(dividerShadow);
    sf::RectangleShape dividerMetal({geometry.boardWidth() + 12.0F, 8.0F});
    dividerMetal.setPosition({geometry.left - 6.0F, dividerY - 4.0F});
    dividerMetal.setFillColor(sf::Color{136, 91, 45, 255});
    window.draw(dividerMetal);
    sf::RectangleShape dividerHighlight({geometry.boardWidth() + 8.0F, 2.0F});
    dividerHighlight.setPosition({geometry.left - 4.0F, dividerY - 1.0F});
    dividerHighlight.setFillColor(sf::Color{224, 178, 103, 255});
    window.draw(dividerHighlight);

    if (state.board.has_value() && state.playerId >= 0) {
        const auto pieceAtDisplay = [&](Position display) -> const PieceSnapshot& {
            const Position logical = PlayerViewTransform::displayToLogical(state.playerId, display);
            return (*state.board)[logical.row][logical.column];
        };
        const auto drawPiece = [&](Position display, bool selected) {
            const PieceSnapshot& piece = pieceAtDisplay(display);
            const float gap = std::max(1.0F, geometry.cellSize * 0.015F);
            const float cellLeft = geometry.left + static_cast<float>(display.column) * geometry.cellSize;
            const float cellTop = geometry.top + static_cast<float>(display.row) * geometry.cellSize;

            if (pieceUsesHeroTexture(piece.type)) {
                heroRenderer_.draw(window,
                                   piece,
                                   {{cellLeft, cellTop}, {geometry.cellSize, geometry.cellSize}},
                                   selected,
                                   font);
                return;
            }

            const BoxTextureState textureState = boxTextureState(selected);
            const float visualScale = boxVisualScale(piece.color, textureState);

            if (pieceUsesBoxTexture(piece.type)) {
                const BoxTextureSet& textureSet = resources_.texturesFor(piece.color);
                const sf::Texture* texture = textureState == BoxTextureState::Selected
                                                 ? textureSet.selected
                                                 : textureSet.normal;
                if (texture != nullptr) {
                    const sf::Vector2u textureSize = texture->getSize();
                    const float availableSize = geometry.cellSize - gap * 2.0F;
                    const float baseScale = std::min(availableSize / static_cast<float>(textureSize.x),
                                                     availableSize / static_cast<float>(textureSize.y));
                    const float scale = baseScale * visualScale;
                    const sf::Vector2f renderedSize{static_cast<float>(textureSize.x) * scale,
                                                    static_cast<float>(textureSize.y) * scale};
                    sf::Sprite sprite(*texture);
                    sprite.setScale({scale, scale});
                    sprite.setPosition({cellLeft + (geometry.cellSize - renderedSize.x) / 2.0F,
                                        cellTop + (geometry.cellSize - renderedSize.y) / 2.0F});
                    window.draw(sprite);
                    return;
                }
            }

            const float fallbackSize = (geometry.cellSize - gap * 2.0F) * visualScale;
            sf::RectangleShape fallback({fallbackSize, fallbackSize});
            fallback.setPosition({cellLeft + (geometry.cellSize - fallbackSize) / 2.0F,
                                  cellTop + (geometry.cellSize - fallbackSize) / 2.0F});
            fallback.setFillColor(pieceColor(piece.color));
            fallback.setOutlineColor(sf::Color{35, 38, 45});
            fallback.setOutlineThickness(-1.0F);
            window.draw(fallback);
        };

        // Normal Boxes form the base layer. Heroes are drawn afterward so their
        // silhouette is never covered by the next cell's tile.
        for (int row = 0; row < BoardRows; ++row) {
            for (int column = 0; column < BoardColumns; ++column) {
                const Position display{row, column};
                if (!input.isSelected(display) && pieceAtDisplay(display).type == PieceType::Box) {
                    drawPiece(display, false);
                }
            }
        }
        for (int row = 0; row < BoardRows; ++row) {
            for (int column = 0; column < BoardColumns; ++column) {
                const Position display{row, column};
                if (!input.isSelected(display) && pieceAtDisplay(display).type == PieceType::Hero) {
                    drawPiece(display, false);
                }
            }
        }
        for (const Position display : input.path()) {
            if (pieceAtDisplay(display).type == PieceType::Box) {
                drawPiece(display, true);
            }
        }
        for (const Position display : input.path()) {
            if (pieceAtDisplay(display).type == PieceType::Hero) {
                drawPiece(display, true);
            }
        }
    }

    if (input.path().size() >= 2U) {
        sf::VertexArray line(sf::PrimitiveType::LineStrip, input.path().size());
        for (std::size_t index = 0; index < input.path().size(); ++index) {
            line[index].position = centerOf(input.path()[index], geometry);
            line[index].color = sf::Color{240, 205, 135, 150};
        }
        window.draw(line);
    }

    for (std::size_t index = 0; index < input.path().size(); ++index) {
        const Position display = input.path()[index];
        const float badgeRadius = std::max(8.0F, geometry.cellSize * 0.13F);
        const float cellLeft = geometry.left + static_cast<float>(display.column) * geometry.cellSize;
        const float cellTop = geometry.top + static_cast<float>(display.row) * geometry.cellSize;
        sf::CircleShape badge(badgeRadius);
        badge.setPosition({cellLeft + 5.0F, cellTop + 5.0F});
        badge.setFillColor(sf::Color{20, 22, 26, 205});
        badge.setOutlineColor(sf::Color{234, 203, 139});
        badge.setOutlineThickness(1.0F);
        window.draw(badge);
        drawText(window, font, std::to_string(index + 1U),
                 std::max(11U, static_cast<unsigned>(badgeRadius * 1.15F)),
                 {cellLeft + badgeRadius * 0.72F, cellTop + badgeRadius * 0.36F});
    }

    if (!input.path().empty()) {
        const sf::Vector2f bannerSize{150.0F, 42.0F};
        sf::RectangleShape banner(bannerSize);
        banner.setPosition({geometry.left + geometry.boardWidth() + 18.0F, dividerY - bannerSize.y / 2.0F});
        banner.setFillColor(sf::Color{38, 29, 25, 235});
        banner.setOutlineColor(sf::Color{205, 157, 84});
        banner.setOutlineThickness(2.0F);
        window.draw(banner);
        const std::string selectionStatus = state.requestPending
                                                ? "处理中"
                                                : "已选择：" + std::to_string(input.path().size());
        drawText(window, font, selectionStatus, 18U,
                 {banner.getPosition().x + 14.0F, banner.getPosition().y + 9.0F},
                 sf::Color{246, 222, 174});
    }

    drawText(window, font, "对方区域", 18U, {geometry.left + geometry.boardWidth() + 18.0F, geometry.top + 10.0F},
             sf::Color{220, 175, 175});
    drawText(window, font, "你的区域", 18U,
             {geometry.left + geometry.boardWidth() + 18.0F, dividerY + 28.0F},
             sf::Color{148, 224, 239});

    if (state.playerId >= 0 && state.board.has_value()) {
        const int enemyPlayerId = 1 - state.playerId;
        const TowerSnapshot& enemyTower = state.towers[static_cast<std::size_t>(enemyPlayerId)];
        const TowerSnapshot& ownTower = state.towers[static_cast<std::size_t>(state.playerId)];
        drawText(window, font,
                 "敌方高塔：" + std::to_string(enemyTower.currentHp) + " / " + std::to_string(enemyTower.maxHp),
                 17U, {geometry.left + geometry.boardWidth() + 18.0F, geometry.top + 42.0F},
                 sf::Color{235, 154, 154});
        drawTowerBar(window, enemyTower,
                     {geometry.left + geometry.boardWidth() + 18.0F, geometry.top + 68.0F},
                     sf::Color{192, 64, 64, 230});
        statusRenderer_.drawTowerStatuses(
            window,
            enemyTower,
            {{geometry.left + geometry.boardWidth() + 18.0F, geometry.top + 88.0F},
             {170.0F, 30.0F}},
            font);
        drawText(window, font,
                 "己方高塔：" + std::to_string(ownTower.currentHp) + " / " + std::to_string(ownTower.maxHp),
                 17U, {geometry.left + geometry.boardWidth() + 18.0F, dividerY + 60.0F},
                 sf::Color{145, 222, 239});
        drawTowerBar(window, ownTower,
                     {geometry.left + geometry.boardWidth() + 18.0F, dividerY + 86.0F},
                     sf::Color{52, 159, 185, 230});
        statusRenderer_.drawTowerStatuses(
            window,
            ownTower,
            {{geometry.left + geometry.boardWidth() + 18.0F, dividerY + 106.0F},
             {170.0F, 30.0F}},
            font);
    }

    if (!input.path().empty() && state.board.has_value() && state.playerId >= 0) {
        const Position logical = PlayerViewTransform::displayToLogical(state.playerId, input.path().back());
        const PieceSnapshot& selectedPiece = (*state.board)[logical.row][logical.column];
        if (selectedPiece.type == PieceType::Hero) {
            const sf::Vector2f detailsPosition{geometry.left + geometry.boardWidth() + 18.0F, dividerY + 142.0F};
            drawText(window, font, heroName(selectedPiece.heroType),
                     16U, detailsPosition, sf::Color{238, 220, 176});
            drawText(window, font, "属性值：" + std::to_string(selectedPiece.attributeValue),
                     16U, {detailsPosition.x, detailsPosition.y + 25.0F});
            drawText(window, font, "生命：" + std::to_string(selectedPiece.currentHp) + " / " +
                                 std::to_string(selectedPiece.maxHp),
                     16U, {detailsPosition.x, detailsPosition.y + 50.0F});
            drawText(window, font, "基础攻击：" + std::to_string(selectedPiece.currentBaseAttackDamage),
                     16U, {detailsPosition.x, detailsPosition.y + 75.0F});
            drawText(window, font, "护盾：" + std::to_string(selectedPiece.shield),
                     16U, {detailsPosition.x, detailsPosition.y + 100.0F});
            drawText(window, font, "易伤：" + std::to_string(selectedPiece.vulnerableLayers) +
                                 "  虚弱：" + std::to_string(selectedPiece.weakLayers),
                     16U, {detailsPosition.x, detailsPosition.y + 125.0F});
            if (selectedPiece.heroType == HeroType::Regent) {
                drawText(window, font, "辉星：" + std::to_string(selectedPiece.radiantStars),
                         16U, {detailsPosition.x, detailsPosition.y + 150.0F});
            } else if (selectedPiece.heroType == HeroType::ChickenPot) {
                drawText(window, font, "闪电充能球：" + std::to_string(selectedPiece.lightningOrbs),
                         16U, {detailsPosition.x, detailsPosition.y + 150.0F});
            }
        }
    }

    const std::string playerText = state.playerId >= 0 ? "玩家编号：" + std::to_string(state.playerId) : "玩家编号：等待分配";
    drawText(window, font, playerText, 20U, {24.0F, 20.0F});
    drawText(window, font, "当前玩家：" +
                 (state.currentPlayerId >= 0 ? std::to_string(state.currentPlayerId) : std::string("-")),
             20U, {24.0F, 50.0F});
    drawText(window, font, "回合 ID：" + std::to_string(state.turnId), 20U, {24.0F, 80.0F});
    drawText(window, font,
             "阶段：" + (state.openingTurnPending ? std::string{"开局布阵"} : phaseText(state.gamePhase)),
             20U, {24.0F, 110.0F});
    drawText(window, font, state.status, 20U, {24.0F, 145.0F}, sf::Color{225, 225, 230});
    if (!state.combatEvents.empty()) {
        drawText(window, font, "战斗日志（服务器顺序）", 17U, {24.0F, 178.0F}, sf::Color{238, 202, 132});
        constexpr std::size_t maximumVisibleEvents = 24U;
        const std::size_t first = state.combatEvents.size() > maximumVisibleEvents
                                      ? state.combatEvents.size() - maximumVisibleEvents
                                      : 0U;
        float logY = 207.0F;
        for (std::size_t index = first; index < state.combatEvents.size(); ++index) {
            drawText(window, font,
                     std::to_string(index + 1U) + ". " + combatEventText(state.combatEvents[index]),
                     14U, {24.0F, logY}, sf::Color{220, 216, 205});
            logY += 22.0F;
        }
    }
    if (!state.lastError.empty()) {
        drawText(window, font, state.lastError, 17U, {24.0F, 750.0F}, sf::Color{245, 105, 105});
    }

    drawButton(window, font, clearButtonBounds(window), "清除选择 / Esc", !input.path().empty());
    drawButton(window, font, confirmButtonBounds(window), "确认消除", input.canConfirm(state));
    window.display();
}

} // namespace sts
