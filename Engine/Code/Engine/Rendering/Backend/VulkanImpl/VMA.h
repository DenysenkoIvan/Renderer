#pragma once

#include <vk_mem_alloc.h>

#include "VkContext.h"

class VMA
{
public:
    static void Initialize();

    static void Deinitialize();

    static VmaAllocator Allocator();

private:
    static VmaAllocator s_allocator;
};