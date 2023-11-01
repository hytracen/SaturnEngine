#pragma once

#include "spdlog/common.h"

#include <engine_pch.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>


namespace saturn {

class Log {
public:
    static auto Ins() -> Log & {
        static Log ins;
        return ins;
    }

    Log(const Log &log) = delete;

    static auto GetEngineLogger() -> std::shared_ptr<spdlog::logger> {
        return m_engine_logger;
    }

    static auto GetGameLogger() -> std::shared_ptr<spdlog::logger> {
        return m_game_logger;
    }


private:
    Log();

    static std::shared_ptr<spdlog::logger> m_engine_logger;
    static std::shared_ptr<spdlog::logger> m_game_logger;
};

}// namespace saturn

#define ENGINE_LOG_TRACE(...) saturn::Log::Ins().GetEngineLogger()->trace(__VA_ARGS__)
#define ENGINE_LOG_INFO(...) saturn::Log::Ins().GetEngineLogger()->info(__VA_ARGS__)
#define ENGINE_LOG_WARN(...) saturn::Log::Ins().GetEngineLogger()->warn(__VA_ARGS__)
#define ENGINE_LOG_ERROR(...) saturn::Log::Ins().GetEngineLogger()->error(__VA_ARGS__)
#define ENGINE_LOG_CRITICAL(...) saturn::Log::Ins().GetEngineLogger()->critical(__VA_ARGS__)

#define GAME_LOG_TRACE(...) saturn::Log::Ins().GetGameLogger()->trace(__VA_ARGS__)
#define GAME_LOG_INFO(...) saturn::Log::Ins().GetGameLogger()->info(__VA_ARGS__)
#define GAME_LOG_WARN(...) saturn::Log::Ins().GetGameLogger()->warn(__VA_ARGS__)
#define GAME_LOG_ERROR(...) saturn::Log::Ins().GetGameLogger()->error(__VA_ARGS__)
#define GAME_LOG_CRITICAL(...) saturn::Log::Ins().GetGameLogger()->critical(__VA_ARGS__)

// 自定义断言
#define SATURN_ASSERT(expression, message) \
    if (!(expression)) { \
        saturn::Log::Ins().GetEngineLogger()->critical("Assertion failed in {} line {}\nExpression:{}\nMessage:{}", __FILE__, __LINE__, #expression, message); \
        abort(); \
    }