#include "GPUQueryPoolTimestamp.h"

void GPUQueryPoolTimestamp::Create()
{
    m_queryCount = 512;

    VkQueryPoolCreateInfo queryPoolInfo{ .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolInfo.queryCount = m_queryCount;

    VK_VALIDATE(vkCreateQueryPool(VkContext::Get()->GetVkDevice(), &queryPoolInfo, nullptr, &m_pool));
}

void GPUQueryPoolTimestamp::Destroy()
{
    m_queryCount = 0;
    m_currentQuery = 0;

    if (!m_pool)
    {
        return;
    }

    vkDestroyQueryPool(VkContext::Get()->GetVkDevice(), m_pool, nullptr);
    m_pool = VK_NULL_HANDLE;
}

void GPUQueryPoolTimestamp::Reset()
{
    m_currentQuery = 0;
    vkResetQueryPool(VkContext::Get()->GetVkDevice(), m_pool, 0, m_queryCount);
}

int GPUQueryPoolTimestamp::GetNewQuery()
{
    Assert(m_currentQuery < m_queryCount);

    return m_currentQuery++;
}

int GPUQueryPoolTimestamp::GetCount()
{
    return m_queryCount;
}

std::vector<uint64_t> GPUQueryPoolTimestamp::GetResults()
{
    if (m_currentQuery == 0)
    {
        return {};
    }

    std::vector<uint64_t> timestamps;
    timestamps.resize(m_currentQuery);

    VK_VALIDATE(vkGetQueryPoolResults(
        VkContext::Get()->GetVkDevice(),
        m_pool,
        0, m_currentQuery,
        m_currentQuery * sizeof(uint64_t), timestamps.data(), sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT));

    return timestamps;
}

VkQueryPool GPUQueryPoolTimestamp::GetVkQueryPool() const
{
    return m_pool;
}