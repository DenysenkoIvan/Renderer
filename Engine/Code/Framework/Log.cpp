#include "Log.h"

#include <format>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>

void InitializeLogger()
{
    auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("Debug", sink);
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%l] %v");

    spdlog::set_default_logger(logger);
}