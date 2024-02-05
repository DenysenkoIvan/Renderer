#pragma once

#include <memory>

#include <Framework/Common.h>

#include "CommandBuffer.h"

#include "VulkanImpl/VkContext.h"
#include "VulkanImpl/VulkanBuffer.h"

class DescriptorBindingTable;
using DescriptorBindingTablePtr = std::unique_ptr<DescriptorBindingTable>;

class DescriptorBindingTable
{
public:
    NON_COPYABLE_MOVABLE(DescriptorBindingTable);

    DescriptorBindingTable();
    ~DescriptorBindingTable();

    void Init();

    void Bind(CommandBufferPtr cmdBuffer);

    void NewFrame(CommandBufferPtr cmdBuffer);

    size_t GetBufferSize() const;
    VkBuffer GetVkBuffer() const;

    VkPipelineLayout GetPipelineLayout() const;

    int GetSampledImageFreeSlot(const void* descriptor);
    int GetStorageImageFreeSlot(const void* descriptor);
    int GetStorageBufferFreeSlot(const void* descriptor);
    int GetSamplerFreeSlot(const void* descriptor);

    void ReleaseSampledImageSlot(int slot);
    void ReleaseStorageImageSlot(int slot);
    void ReleaseStorageBufferSlot(int slot);
    void ReleaseSamplerSlot(int slot);

    int GetSampledImageDescriptorSize() const;
    int GetStorageImageDescriptorSize() const;
    int GetStorageBufferDescriptorSize() const;
    int GetSamplerDescriptorSize() const;

    static DescriptorBindingTablePtr Create();
    static DescriptorBindingTable* Get();

private:
    void CreateLayouts();
    void CreateBuffer();
    void RetrieveBufferAddress();
    void RetrieveDescriptorSizes();

    void UpdateBuffer(size_t offset, const void* data, size_t size);

private:
    int m_sampledImageMaxCount = 1024 * 1024;
    int m_storageImageMaxCount = 1024 * 1024;
    int m_storageBufferMaxCount = 1024 * 1024;
    int m_samplerMaxCount = 16 * 1024;

    int m_sampledImageStartSlot = 0;
    int m_storageImageStartSlot = m_sampledImageMaxCount;
    int m_storageBufferStartSlot = m_storageImageStartSlot + m_storageImageMaxCount;
    int m_samplerStartSlot = m_storageBufferStartSlot + m_storageBufferMaxCount;

    int m_sampledImageDescriptorSize = 0;
    int m_storageImageDescriptorSize = 0;
    int m_storageBufferDescriptorSize = 0;
    int m_samplerDescriptorSize = 0;

    int m_boundSampledImageCount = 1;
    int m_boundStorageImageCount = 1;
    int m_boundStorageBufferCount = 1;
    int m_boundSamplerCount = 1;

    VkDeviceAddress m_sampledImageMemoryOffset = 0;
    VkDeviceAddress m_storageImageMemoryOffset = 0;
    VkDeviceAddress m_storageBufferMemoryOffset = 0;
    VkDeviceAddress m_samplerMemoryOffset = 0;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    VulkanBuffer m_buffer;
    VkDeviceAddress m_bufferAddress = 0;

    CommandBufferPtr m_cmdBuffer;

    static DescriptorBindingTable* s_dbt;
};