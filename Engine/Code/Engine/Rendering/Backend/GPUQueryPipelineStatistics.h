#pragma once

#include <vulkan/vulkan.h>

#include "VulkanImpl/VkContext.h"
#include "VulkanImpl/VulkanValidation.h"

struct PipelineStatistics
{
    std::string name;
    int count = 0;

    PipelineStatistics(const char* name);
};

class GPUQueryPipelineStatistics
{
public:
    void Create();
    void Destroy();

    void Reset();

    std::vector<PipelineStatistics> GetResults();

    VkQueryPool GetVkQueryPool() const;

private:
    VkQueryPool m_pipelineStatsPool = VK_NULL_HANDLE;
    int m_pipelineStatsQueryCount = 0;
};