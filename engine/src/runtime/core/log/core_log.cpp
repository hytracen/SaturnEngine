#include "core_log.hpp"
#include "spdlog/common.h"

namespace saturn {

std::shared_ptr<spdlog::logger> Log::m_engine_logger;
std::shared_ptr<spdlog::logger> Log::m_game_logger;

Log::Log() {
    spdlog::set_pattern("%^[%T %n] %l: %v%$");
    m_engine_logger = spdlog::stderr_color_mt("SaturnEngine");
    m_game_logger = spdlog::stderr_color_mt("Game");
}

}// namespace saturn