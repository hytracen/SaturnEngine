#pragma once

#include <engine_pch.hpp>
#include <runtime/function/rendering/render_window.hpp>

namespace saturn {

class Game {
public:
    Game(int width, int height);

    ~Game();

    void Run();

private:
    int m_width, m_height;
    std::shared_ptr<RenderWindow> m_window;
};

}// namespace saturn
