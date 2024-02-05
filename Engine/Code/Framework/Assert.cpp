#include "Assert.h"

#include "Log.h"

#if defined(ASSERT_ENABLE)

void Assert(bool expression, std::source_location src)
{
    if (!expression)
    {
        LogError("{}.{}: Assertion failed", src.file_name(), src.line());
        __debugbreak();
    }
}

void Assert(bool expression, const char* message, std::source_location src)
{
    if (!expression)
    {
        LogError("{}.{}: Assertion failed", src.file_name(), src.line());
        __debugbreak();
    }
}

void Error(bool expression, std::source_location src)
{
    if (!expression)
    {
        LogError("{}.{}: Assertion failed", src.file_name(), src.line());
    }
}

void Error(bool expression, const char* message, std::source_location src)
{
    if (!expression)
    {
        LogError("{}.{}: Assertion failed", src.file_name(), src.line());
    }
}

#endif