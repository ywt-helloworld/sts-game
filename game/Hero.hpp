#pragma once

#include "game/BoardPiece.hpp"

#include <algorithm>

namespace sts {

class Hero : public BoardPiece {
public:
    Hero(PieceColor color, Position position, int attribute, int attack, int defense)
        : BoardPiece(color, position), attribute_(std::max(0, attribute)), attack_(attack), defense_(defense) {}
    ~Hero() override = default;

    [[nodiscard]] PieceType type() const noexcept override { return PieceType::Hero; }
    [[nodiscard]] int inheritedAttribute() const noexcept override { return attribute_; }
    [[nodiscard]] int attackValue() const noexcept { return attack_; }
    [[nodiscard]] int defenseValue() const noexcept { return defense_; }
    [[nodiscard]] bool isDead() const noexcept { return attribute_ == 0; }

    void modifyAttribute(int delta) noexcept { attribute_ = std::max(0, attribute_ + delta); }
    void receiveDamage(int damage) noexcept { modifyAttribute(-std::max(0, damage - defense_)); }
    void attack(Hero& target) const noexcept { target.receiveDamage(attack_); }
    void setAttackValue(int value) noexcept { attack_ = std::max(0, value); }
    void setDefenseValue(int value) noexcept { defense_ = std::max(0, value); }

    void onSpawn() override { spawned_ = true; }
    void onEliminated() override { eliminated_ = true; }
    virtual void useSkill(Hero& target) = 0;

private:
    int attribute_{};
    int attack_{};
    int defense_{};
    bool spawned_{};
    bool eliminated_{};
};

class RedHero final : public Hero {
public:
    RedHero(Position position, int attribute) : Hero(PieceColor::Red, position, attribute, 12, 2) { onSpawn(); }
    void useSkill(Hero& target) override { target.receiveDamage(3); }
};

class YellowHero final : public Hero {
public:
    YellowHero(Position position, int attribute) : Hero(PieceColor::Yellow, position, attribute, 9, 4) { onSpawn(); }
    void useSkill(Hero&) override { setDefenseValue(defenseValue() + 1); }
};

class GreenHero final : public Hero {
public:
    GreenHero(Position position, int attribute) : Hero(PieceColor::Green, position, attribute, 8, 3) { onSpawn(); }
    void useSkill(Hero&) override { modifyAttribute(2); }
};

class BlueHero final : public Hero {
public:
    BlueHero(Position position, int attribute) : Hero(PieceColor::Blue, position, attribute, 10, 3) { onSpawn(); }
    void useSkill(Hero&) override { setAttackValue(attackValue() + 1); }
};

} // namespace sts
