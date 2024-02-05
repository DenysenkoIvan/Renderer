#include "GPUQueryPipelineStatistics.h"

PipelineStatistics::PipelineStatistics(const char* name)
    : name(name), count(0) {}

void GPUQueryPipelineStatistics::Create()
{
    VkQueryPoolCreateInfo pipelineStatsQueryPoolInfo{ .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    pipelineStatsQueryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    pipelineStatsQueryPoolInfo.pipelineStatistics =
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
        //VK_QUERY_PIPELINE_STATISTIC_TASK_SHADER_INVOCATIONS_BIT_EXT |
        //VK_QUERY_PIPELINE_STATISTIC_MESH_SHADER_INVOCATIONS_BIT_EXT;
    pipelineStatsQueryPoolInfo.queryCount = m_pipelineStatsQueryCount = 7;

    VK_VALIDATE(vkCreateQueryPool(VkContext::Get()->GetVkDevice(), &pipelineStatsQueryPoolInfo, nullptr, &m_pipelineStatsPool));
}

void GPUQueryPipelineStatistics::Destroy()
{
    vkDestroyQueryPool(VkContext::Get()->GetVkDevice(), m_pipelineStatsPool, nullptr);
    m_pipelineStatsPool = VK_NULL_HANDLE;
}

void GPUQueryPipelineStatistics::Reset()
{
    vkResetQueryPool(VkContext::Get()->GetVkDevice(), m_pipelineStatsPool, 0, m_pipelineStatsQueryCount);
}

std::vector<PipelineStatistics> GPUQueryPipelineStatistics::GetResults()
{
    std::vector<uint64_t> results(m_pipelineStatsQueryCount);

    vkGetQueryPoolResults(
        VkContext::Get()->GetVkDevice(),
        m_pipelineStatsPool,
        0, 1,
        m_pipelineStatsQueryCount * sizeof(uint64_t), results.data(), sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT);

    std::vector<PipelineStatistics> resultStrs =
    {
        "Vertex count        ",
        "Primitives count    ",
        "VS invocations      ",
        "Primitives processed",
        "Primitives output   ",
        "FS invocations      ",
        "CS invocations      "
        //"TS invocations      ",
        //"MS invocations      "
    };

    int i = 0;
    for (PipelineStatistics& statistics : resultStrs)
    {
        statistics.count = (int)results[i++];
    }

    return resultStrs;
}

VkQueryPool GPUQueryPipelineStatistics::GetVkQueryPool() const
{
    return m_pipelineStatsPool;
}