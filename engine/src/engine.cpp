#include "engine.hpp"
namespace saturn {
Engine::Engine() { Init(); }

Engine::~Engine() = default;

void Engine::Init() { m_render_system = std::make_unique<rendering::RenderSystem>(1920, 1080); }

void Engine::Run() {
    while (!m_render_system->ShouldCloseWindow()) {
        float delta_time = CalculateDeltaTime();
        glfwPollEvents();
        m_render_system->Tick(delta_time);
    }
}

auto Engine::CalculateDeltaTime() -> float {
    float delta_time;
    std::chrono::steady_clock::time_point tick_time_point = std::chrono::steady_clock::now();
    auto time_span = duration_cast<std::chrono::duration<float>>(tick_time_point - m_last_tick_time_point);
    delta_time = time_span.count();

    m_last_tick_time_point = tick_time_point;
    return delta_time;
}

}// namespace saturn