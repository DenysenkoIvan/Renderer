#pragma once

#include <source_location>

#if defined(ASSERT_ENABLE)

void Assert(bool expression, std::source_location src = std::source_location::current());

void Assert(bool expression, const char* message, std::source_location src = std::source_location::current());

void Error(bool expression, std::source_location src = std::source_location::current());

void Error(bool expression, const char* message, std::source_location src = std::source_location::current());

#else

#define Assert(...)
#define Error(...)

#endif