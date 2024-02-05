#pragma once

#include "VMA.h"

#include <memory>

class VulkanBuffer
{
public:
    VulkanBuffer() = default;
    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    void Create(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperty,
        VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE, uint32_t queueFamilyIndexCount = 0, const uint32_t* pQueueFamilyIndices = nullptr);

    size_t GetSize() const;

    void* Map();
    void Unmap();

    VkBuffer GetVkBuffer() const;

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    size_t m_size = 0;
    VmaAllocation m_vmaAllocation = VK_NULL_HANDLE;
    VmaAllocationInfo m_vmaAllocationInfo{};
};