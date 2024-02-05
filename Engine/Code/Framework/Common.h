#pragma once

#include "Assert.h"
#include "Log.h"
#include "Profile.h"
#include "String.h"

#define NON_COPYABLE_MOVABLE(class)          \
    class(const class&) = delete;            \
    class(class&&) = delete;                 \
    class& operator=(const class&) = delete; \
    class& operator=(class&&) = delete;