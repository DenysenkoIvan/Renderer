#include "VulkanBuffer.h"

#include "VulkanValidation.h"

VulkanBuffer::~VulkanBuffer()
{
    Assert(m_buffer != VK_NULL_HANDLE && m_vmaAllocation != VK_NULL_HANDLE);

    vmaDestroyBuffer(VMA::Allocator(), m_buffer, m_vmaAllocation);
    m_buffer = VK_NULL_HANDLE;
    m_vmaAllocation = VK_NULL_HANDLE;
    m_vmaAllocationInfo = {};
}

void VulkanBuffer::Create(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperty,
    VkSharingMode sharingMode, uint32_t queueFamilyIndexCount, const uint32_t* pQueueFamilyIndices)
{
    m_size = size;

    VkBufferCreateInfo bufferInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = m_size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = sharingMode;
    bufferInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    bufferInfo.pQueueFamilyIndices = pQueueFamilyIndices;

    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.flags |= (memProperty & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        ? 0
        : VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocationCreateInfo.requiredFlags = memProperty;

    VK_VALIDATE(vmaCreateBuffer(VMA::Allocator(), &bufferInfo, &allocationCreateInfo, &m_buffer, &m_vmaAllocation, &m_vmaAllocationInfo));
}

size_t VulkanBuffer::GetSize() const
{
    return m_size;
}

void* VulkanBuffer::Map()
{
    void* mappedData = nullptr;
    VK_VALIDATE(vmaMapMemory(VMA::Allocator(), m_vmaAllocation, &mappedData));
    return mappedData;
}

void VulkanBuffer::Unmap()
{
    vmaUnmapMemory(VMA::Allocator(), m_vmaAllocation);
}

VkBuffer VulkanBuffer::GetVkBuffer() const
{
    return m_buffer;
}