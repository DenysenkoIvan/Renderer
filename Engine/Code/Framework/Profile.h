#pragma once

#include <Tracy.hpp>
#include <../common/TracySystem.hpp>

#if defined(PROFILE_ENABLE)
#define ProfileFunction() ZoneScoped
#define ProfileScope(name) ZoneScopedN(name);
#define ProfileFrame() FrameMark
#define ProfileStall(name) ZoneScopedNC(name, 0x00FF0000) // Red color
#define ProfileSetThreadName(threadName) tracy::SetThreadName(threadName);
#else
#define ProfileFunction()
#define ProfileScope(name)
#define ProfileFrame()
#define ProfileStall(name)
#define ProfileSetThreadName(threadName)
#endif