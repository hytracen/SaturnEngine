#include "core_log.hpp"
#include "spdlog/common.h"

namespace saturn {

std::shared_ptr<spdlog::logger> Log::m_engine_logger;
std::shared_ptr<spdlog::logger> Log::m_game_logger;

Log::Log() {
    spdlog::set_pattern("%^[%T %n][%l] %v%$");
    m_engine_logger = spdlog::stderr_color_mt("SaturnEngine");
    m_engine_logger->set_level(spdlog::level::level_enum::trace); // set_level(min_level)只显示min_level及比min_level更高的信息

    m_game_logger = spdlog::stderr_color_mt("Game");
    m_game_logger->set_level(spdlog::level::level_enum::trace);
}

}// namespace saturn