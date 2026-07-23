#include "gui/GameWindow.hpp"

#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowEnums.hpp>

namespace sts {

GameWindow::GameWindow()
    : window_(sf::VideoMode({2000U, 1500U}), "STS Match - Graphical Client", sf::Style::Default) {
    window_.setFramerateLimit(60U);
}

bool GameWindow::isOpen() const noexcept {
    return window_.isOpen();
}

std::optional<sf::Event> GameWindow::pollEvent() {
    return window_.pollEvent();
}

void GameWindow::close() noexcept {
    window_.close();
}

sf::RenderWindow& GameWindow::renderWindow() noexcept {
    return window_;
}

} // namespace sts
