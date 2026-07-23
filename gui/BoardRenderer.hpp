#pragma once

#include "gui/BoardGeometry.hpp"
#include "gui/ClientGameState.hpp"
#include "gui/HeroRenderer.hpp"
#include "gui/InputController.hpp"
#include "gui/ResourceManager.hpp"
#include "gui/StatusEffectRenderer.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

namespace sts {

class BoardRenderer {
public:
    BoardRenderer();

    void draw(sf::RenderWindow& window,
              const ClientGameState& state,
              const InputController& input,
              const BoardGeometry& geometry);

    [[nodiscard]] sf::FloatRect confirmButtonBounds(const sf::RenderWindow& window) const noexcept;
    [[nodiscard]] sf::FloatRect clearButtonBounds(const sf::RenderWindow& window) const noexcept;

private:
    ResourceManager resources_;
    HeroRenderer heroRenderer_;
    StatusEffectRenderer statusRenderer_;
};

} // namespace sts
