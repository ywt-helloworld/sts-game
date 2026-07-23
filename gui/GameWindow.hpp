#pragma once

#include <SFML/Graphics/RenderWindow.hpp>

#include <optional>

namespace sts {

class GameWindow {
public:
    GameWindow();

    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] std::optional<sf::Event> pollEvent();
    void close() noexcept;
    [[nodiscard]] sf::RenderWindow& renderWindow() noexcept;

private:
    sf::RenderWindow window_;
};

} // namespace sts
