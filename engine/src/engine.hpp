#pragma once

#include <engine_pch.hpp>

#include <runtime/function/rendering/render_system.hpp>

namespace saturn {

class Engine {
public:
    Engine();
    ~Engine();

    void Run();

    [[nodiscard]] auto CalculateDeltaTime() -> float;

private:
    void Init();

    void Clear();

    std::unique_ptr<rendering::RenderSystem> m_render_system;

    std::chrono::steady_clock::time_point m_last_tick_time_point{std::chrono::steady_clock::now()};
};

}// namespace saturn