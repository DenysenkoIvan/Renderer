#include "Buffer.h"

#include <Framework/Common.h>

#include <Engine/Rendering/Renderer.h>

#include "DescriptorBindingTable.h"

#include "VulkanImpl/VulkanValidation.h"

Buffer::Buffer(BufferUsageFlags usage, int64_t size, bool onGpu)
    : m_usage(usage)
{
    VkBufferUsageFlags vkUsage = 0;
    if (usage & BufferUsageTransferSrc)
    {
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (usage & BufferUsageTransferDst)
    {
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (usage & (BufferUsageStorageRead | BufferUsageStorageWrite))
    {
        vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    VkMemoryPropertyFlags memProperty = onGpu ?
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT :
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    m_buffer.Create(size, vkUsage, memProperty);
}

Buffer::~Buffer()
{
    if (m_descriptorSlot != -1)
    {
        DescriptorBindingTable::Get()->ReleaseStorageBufferSlot(m_descriptorSlot);
        m_descriptorSlot = -1;
    }
}

void Buffer::SetName(std::string_view name)
{
#if !defined(RELEASE_BUILD)
    m_name = name;

    VkDebugUtilsObjectNameInfoEXT nameInfo{ .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
    nameInfo.objectHandle = (uint64_t)m_buffer.GetVkBuffer();
    nameInfo.pObjectName = m_name.c_str();

    VK_VALIDATE(VkContext::Get()->GetProcAddresses().vkSetDebugUtilsObjectName(VkContext::Get()->GetVkDevice(), &nameInfo));
#endif
}

const std::string& Buffer::GetName() const
{
#if !defined(RELEASE_BUILD)
    return m_name;
#else
    static std::string empty;
    return empty;
#endif
}

int64_t Buffer::GetSize() const
{
    return (int64_t)m_buffer.GetSize();
}

int Buffer::BindSRV()
{
    if (m_descriptorSlot != -1)
    {
        return m_descriptorSlot;
    }

    BindDescriptor();

    return m_descriptorSlot;
}

int Buffer::BindUAV()
{
    if (m_descriptorSlot != -1)
    {
        return m_descriptorSlot;
    }

    BindDescriptor();

    return m_descriptorSlot;
}

void* Buffer::Map()
{
    return m_buffer.Map();
}

void Buffer::Unmap()
{
    m_buffer.Unmap();
}

VulkanBuffer& Buffer::GetBuffer()
{
    return m_buffer;
}

BufferPtr Buffer::CreateStaging(int64_t size)
{
    Assert(size > 0);

    return std::make_shared<Buffer>(BufferUsageTransferSrc | BufferUsageTransferDst, size, false);
}

BufferPtr Buffer::CreateStructured(int64_t size, bool isWritable)
{
    Assert(size > 0);

    BufferUsageFlags usage = BufferUsageTransferDst | BufferUsageStorageRead;
    if (isWritable)
    {
        usage |= BufferUsageStorageWrite;
    }

    return std::make_shared<Buffer>(usage, size, true);
}

void Buffer::BindDescriptor()
{
    VkBufferDeviceAddressInfo addressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    addressInfo.buffer = m_buffer.GetVkBuffer();

    VkDescriptorAddressInfoEXT bufferInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
    bufferInfo.address = vkGetBufferDeviceAddress(VkContext::Get()->GetVkDevice(), &addressInfo);
    bufferInfo.range = m_buffer.GetSize();

    VkDescriptorGetInfoEXT info{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
    info.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    info.data.pStorageBuffer = &bufferInfo;

    uint8_t descriptor[64]{};
    VkContext::Get()->GetProcAddresses().vkGetDescriptor(VkContext::Get()->GetVkDevice(), &info, DescriptorBindingTable::Get()->GetStorageBufferDescriptorSize(), descriptor);

    m_descriptorSlot = DescriptorBindingTable::Get()->GetStorageBufferFreeSlot(descriptor);
    
    Assert(m_descriptorSlot != -1);
}