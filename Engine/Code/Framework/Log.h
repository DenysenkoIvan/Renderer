#pragma once

#undef SPDLOG_HEADER_ONLY
#include <spdlog/spdlog.h>

void InitializeLogger();

#define LogAlways(...) spdlog::trace(__VA_ARGS__);

#if defined(LOG_ENABLE)
#define Log(...) spdlog::trace(__VA_ARGS__);
#define LogInfo(...) spdlog::info(__VA_ARGS__);
#define LogWarning(...) spdlog::warn(__VA_ARGS__);
#define LogError(...) spdlog::error(__VA_ARGS__);
#else
#define Log(...)
#define LogInfo(...)
#define LogWarning(...)
#define LogError(...)
#endif