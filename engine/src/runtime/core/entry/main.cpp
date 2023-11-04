#include <engine.hpp>

auto main() -> int {
    auto engine = std::make_unique<saturn::Engine>();
    engine->Run();
    return 0;
}