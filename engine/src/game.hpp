#pragma once

#include <engine_pch.hpp>
#include <runtime/function/rendering/window.hpp>

namespace saturn {

class Game {
public:
    Game(int width, int height);

    ~Game();

    void Run();

private:
    int m_width, m_height;
    std::shared_ptr<rendering::Window> m_window;
};

}// namespace saturn
