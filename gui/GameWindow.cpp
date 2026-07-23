#include "gui/GameWindow.hpp"

#include "gui/WindowLayout.hpp"

#include <SFML/Graphics/View.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowEnums.hpp>

namespace sts {
namespace {

WindowPlacement initialPlacement() noexcept {
    const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    return fitWindowInsideDesktop(desktop.size.x, desktop.size.y);
}

sf::VideoMode initialVideoMode() noexcept {
    const WindowPlacement placement = initialPlacement();
    return sf::VideoMode({placement.width, placement.height});
}

} // namespace

GameWindow::GameWindow()
    : window_(initialVideoMode(),
              "STS Match - Graphical Client",
              sf::Style::Default) {
    const WindowPlacement placement = initialPlacement();
    window_.setPosition({placement.left, placement.top});
    window_.setView(sf::View(sf::FloatRect{
        {0.0F, 0.0F},
        {static_cast<float>(placement.width), static_cast<float>(placement.height)},
    }));
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
