#pragma once

#include <string>

#include <Framework/Common.h>

#include <vulkan/vulkan.h>

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTEFINAL(x, y) TOKENPASTE(x, y)

#if defined VULKAN_VALIDATION_ENABLE
#define VK_VALIDATE(x)                                                                         \
    VkResult TOKENPASTEFINAL(result, __LINE__) = x;                                                             \
    std::string TOKENPASTEFINAL(message, __LINE__) = "Vulkan return code: " + std::to_string(TOKENPASTEFINAL(result, __LINE__)); \
    Assert(TOKENPASTEFINAL(result, __LINE__) == VK_SUCCESS, TOKENPASTEFINAL(message, __LINE__).c_str());
#else
#define VK_VALIDATE(x) x
#endif