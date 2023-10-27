#include "game.hpp"

namespace saturn {

Game::Game(int width, int height) : m_width(width), m_height(height) {
    ENGINE_LOG_INFO("Start!");
}

void Game::Run() {
    m_window = std::make_shared<RenderWindow>(m_width, m_height, "Saturn Game");
}

}// namespace saturn