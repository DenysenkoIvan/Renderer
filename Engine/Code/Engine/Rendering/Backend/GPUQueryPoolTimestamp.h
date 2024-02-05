#pragma once

#include <vulkan/vulkan.h>

#include "VulkanImpl/VkContext.h"
#include "VulkanImpl/VulkanValidation.h"

class GPUQueryPoolTimestamp
{
public:
    void Create();
    void Destroy();

    void Reset();

    int GetNewQuery();
    int GetCount();

    std::vector<uint64_t> GetResults();

    VkQueryPool GetVkQueryPool() const;

private:
    VkQueryPool m_pool = VK_NULL_HANDLE;
    int m_queryCount = 0;
    int m_currentQuery = 0;
};