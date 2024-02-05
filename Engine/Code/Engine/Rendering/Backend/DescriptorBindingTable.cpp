#include "DescriptorBindingTable.h"

#include "VulkanImpl/VulkanValidation.h"

DescriptorBindingTable* DescriptorBindingTable::s_dbt = nullptr;

DescriptorBindingTable::DescriptorBindingTable()
{
    CreateLayouts();
    CreateBuffer();
    RetrieveBufferAddress();
    RetrieveDescriptorSizes();
}

DescriptorBindingTable::~DescriptorBindingTable()
{
    if (m_pipelineLayout)
    {
        vkDestroyPipelineLayout(VkContext::Get()->GetVkDevice(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(VkContext::Get()->GetVkDevice(), m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
}

void DescriptorBindingTable::Init()
{
    std::vector<uint8_t> zeros(GetBufferSize());

    UpdateBuffer(0, zeros.data(), GetBufferSize());
}

void DescriptorBindingTable::Bind(CommandBufferPtr cmdBuffer)
{
    VkDescriptorBufferBindingInfoEXT info{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT };
    info.address = m_bufferAddress;
    info.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;

    uint32_t bufferIndex = 0;
    VkDeviceSize bufferOffset = 0;

    VkCommandBuffer vkCmdBuffer = cmdBuffer->GetCommandBuffer().GetVkCommandBuffer();

    VkContext::Get()->GetProcAddresses().vkCmdBindDescriptorBuffers(vkCmdBuffer, 1, &info);
    VkContext::Get()->GetProcAddresses().vkCmdSetDescriptorBufferOffsets(vkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &bufferIndex, &bufferOffset);
    VkContext::Get()->GetProcAddresses().vkCmdSetDescriptorBufferOffsets(vkCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &bufferIndex, &bufferOffset);

    cmdBuffer->m_pipelineLayout = m_pipelineLayout;
}

void DescriptorBindingTable::NewFrame(CommandBufferPtr cmdBuffer)
{
    m_cmdBuffer = cmdBuffer;
}

size_t DescriptorBindingTable::GetBufferSize() const
{
    return m_buffer.GetSize();
}

VkBuffer DescriptorBindingTable::GetVkBuffer() const
{
    return m_buffer.GetVkBuffer();
}

VkPipelineLayout DescriptorBindingTable::GetPipelineLayout() const
{
    return m_pipelineLayout;
}

int DescriptorBindingTable::GetSampledImageFreeSlot(const void* descriptor)
{
    Assert(m_boundSampledImageCount + 1 <= m_sampledImageMaxCount);

    int slot = m_boundSampledImageCount++;

    UpdateBuffer(m_sampledImageMemoryOffset + slot * m_sampledImageDescriptorSize, descriptor, m_sampledImageDescriptorSize);

    return slot;
}

int DescriptorBindingTable::GetStorageImageFreeSlot(const void* descriptor)
{
    Assert(m_boundStorageImageCount + 1 <= m_storageImageMaxCount);

    int slot = m_boundStorageImageCount++;

    UpdateBuffer(m_storageImageMemoryOffset + slot * m_storageImageDescriptorSize, descriptor, m_storageImageDescriptorSize);

    return slot;
}

int DescriptorBindingTable::GetStorageBufferFreeSlot(const void* descriptor)
{
    Assert(m_boundStorageBufferCount + 1 <= m_storageBufferMaxCount);

    int slot = m_boundStorageBufferCount++;

    UpdateBuffer(m_storageBufferMemoryOffset + slot * m_storageBufferDescriptorSize, descriptor, m_storageBufferDescriptorSize);

    return slot;
}

int DescriptorBindingTable::GetSamplerFreeSlot(const void* descriptor)
{
    Assert(m_boundSamplerCount + 1 <= m_samplerMaxCount);

    int slot = m_boundSamplerCount++;

    UpdateBuffer(m_samplerMemoryOffset + slot * m_samplerDescriptorSize, descriptor, m_samplerDescriptorSize);

    return slot;
}

void DescriptorBindingTable::ReleaseSampledImageSlot(int slot)
{
    Assert(slot >= 0 && slot < m_sampledImageMaxCount);

    uint8_t zeros[64]{};
    UpdateBuffer(m_sampledImageMemoryOffset + slot * m_sampledImageDescriptorSize, zeros, m_sampledImageDescriptorSize);
}

void DescriptorBindingTable::ReleaseStorageImageSlot(int slot)
{
    Assert(slot >= 0 && slot < m_storageImageMaxCount);

    uint8_t zeros[64]{};
    UpdateBuffer(m_storageImageMemoryOffset + slot * m_storageImageDescriptorSize, zeros, m_storageImageDescriptorSize);
}

void DescriptorBindingTable::ReleaseStorageBufferSlot(int slot)
{
    Assert(slot >= 0 && slot < m_storageBufferMaxCount);

    uint8_t zeros[64]{};
    UpdateBuffer(m_storageBufferMemoryOffset + slot * m_storageBufferDescriptorSize, zeros, m_storageBufferDescriptorSize);
}

void DescriptorBindingTable::ReleaseSamplerSlot(int slot)
{
    Assert(slot >= 0 && slot < m_samplerMaxCount);

    uint8_t zeros[64]{};
    UpdateBuffer(m_samplerMemoryOffset + slot * m_samplerDescriptorSize, zeros, m_samplerDescriptorSize);
}

int DescriptorBindingTable::GetSampledImageDescriptorSize() const
{
    return m_sampledImageDescriptorSize;
}

int DescriptorBindingTable::GetStorageImageDescriptorSize() const
{
    return m_storageImageDescriptorSize;
}

int DescriptorBindingTable::GetStorageBufferDescriptorSize() const
{
    return m_storageBufferDescriptorSize;
}

int DescriptorBindingTable::GetSamplerDescriptorSize() const
{
    return m_samplerDescriptorSize;
}

DescriptorBindingTablePtr DescriptorBindingTable::Create()
{
    Assert(!s_dbt);

    DescriptorBindingTablePtr dbt = std::make_unique<DescriptorBindingTable>();
    
    s_dbt = dbt.get();

    return dbt;
}

DescriptorBindingTable* DescriptorBindingTable::Get()
{
    return s_dbt;
}

void DescriptorBindingTable::CreateLayouts()
{
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
    bindings[0].binding = m_sampledImageStartSlot;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bindings[0].descriptorCount = m_sampledImageMaxCount;
    bindings[0].stageFlags = VK_SHADER_STAGE_ALL;
    bindings[1].binding = m_storageImageStartSlot;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = m_storageImageMaxCount;
    bindings[1].stageFlags = VK_SHADER_STAGE_ALL;
    bindings[2].binding = m_storageBufferStartSlot;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = m_storageBufferMaxCount;
    bindings[2].stageFlags = VK_SHADER_STAGE_ALL;
    bindings[3].binding = m_samplerStartSlot;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    bindings[3].descriptorCount = m_samplerMaxCount;
    bindings[3].stageFlags = VK_SHADER_STAGE_ALL;

    VkDescriptorSetLayoutCreateInfo descriptorSetInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    descriptorSetInfo.bindingCount = (uint32_t)bindings.size();
    descriptorSetInfo.pBindings = bindings.data();

    VK_VALIDATE(vkCreateDescriptorSetLayout(VkContext::Get()->GetVkDevice(), &descriptorSetInfo, nullptr, &m_descriptorSetLayout));

    VkPushConstantRange pushConstants{};
    pushConstants.stageFlags = VK_SHADER_STAGE_ALL;
    pushConstants.size = 64;

    VkPipelineLayoutCreateInfo layoutInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstants;

    VK_VALIDATE(vkCreatePipelineLayout(VkContext::Get()->GetVkDevice(), &layoutInfo, nullptr, &m_pipelineLayout));
}

void DescriptorBindingTable::CreateBuffer()
{
    VkDeviceSize size = 0;
    VkContext::Get()->GetProcAddresses().vkGetDescriptorSetLayoutSize(VkContext::Get()->GetVkDevice(), m_descriptorSetLayout, &size);

    m_buffer.Create(size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void DescriptorBindingTable::RetrieveBufferAddress()
{
    VkBufferDeviceAddressInfo addressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    addressInfo.buffer = m_buffer.GetVkBuffer();

    m_bufferAddress = vkGetBufferDeviceAddress(VkContext::Get()->GetVkDevice(), &addressInfo);

    VkContext::Get()->GetProcAddresses().vkGetDescriptorSetLayoutBindingOffset(VkContext::Get()->GetVkDevice(), m_descriptorSetLayout, m_sampledImageStartSlot, &m_sampledImageMemoryOffset);
    VkContext::Get()->GetProcAddresses().vkGetDescriptorSetLayoutBindingOffset(VkContext::Get()->GetVkDevice(), m_descriptorSetLayout, m_storageImageStartSlot, &m_storageImageMemoryOffset);
    VkContext::Get()->GetProcAddresses().vkGetDescriptorSetLayoutBindingOffset(VkContext::Get()->GetVkDevice(), m_descriptorSetLayout, m_storageBufferStartSlot, &m_storageBufferMemoryOffset);
    VkContext::Get()->GetProcAddresses().vkGetDescriptorSetLayoutBindingOffset(VkContext::Get()->GetVkDevice(), m_descriptorSetLayout, m_samplerStartSlot, &m_samplerMemoryOffset);
}

void DescriptorBindingTable::RetrieveDescriptorSizes()
{
    const VkPhysicalDeviceDescriptorBufferPropertiesEXT& props = VkContext::Get()->GetPhysicalDevice().GetDescriptorBufferProperties();

    m_sampledImageDescriptorSize = (uint32_t)props.sampledImageDescriptorSize;
    m_storageImageDescriptorSize = (uint32_t)props.storageImageDescriptorSize;
    m_storageBufferDescriptorSize = (uint32_t)props.storageBufferDescriptorSize;
    m_samplerDescriptorSize = (uint32_t)props.samplerDescriptorSize;

    int maxSize = std::max(
        std::max(m_sampledImageDescriptorSize, m_storageImageDescriptorSize),
        std::max(m_storageBufferDescriptorSize, m_samplerDescriptorSize));
    
    Assert(maxSize <= 64);
}

void DescriptorBindingTable::UpdateBuffer(size_t offset, const void* data, size_t size)
{
    Assert(m_cmdBuffer && data && size);

    m_cmdBuffer->ValidateIsInRecordingState();

    BufferPtr stagingBuffer = m_cmdBuffer->GetStagingBuffer(size);

    void* stagingBufferPtr = stagingBuffer->Map();
    memcpy(stagingBufferPtr, data, size);
    stagingBuffer->Unmap();

    VkBufferMemoryBarrier2 barrier{ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
    barrier.srcStageMask =
        VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT |
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR |
        VK_PIPELINE_STAGE_2_COPY_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = m_buffer.GetVkBuffer();
    barrier.offset = offset;
    barrier.size = size;

    VkDependencyInfo dependency{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependency.bufferMemoryBarrierCount = 1;
    dependency.pBufferMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(m_cmdBuffer->GetCommandBuffer().GetVkCommandBuffer(), &dependency);

    VkBufferCopy2 region{ .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2 };
    region.srcOffset = 0;
    region.dstOffset = offset;
    region.size = size;

    VkCopyBufferInfo2 copyInfo{ .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2 };
    copyInfo.srcBuffer = stagingBuffer->GetBuffer().GetVkBuffer();
    copyInfo.dstBuffer = m_buffer.GetVkBuffer();
    copyInfo.regionCount = 1;
    copyInfo.pRegions = &region;

    vkCmdCopyBuffer2(m_cmdBuffer->GetCommandBuffer().GetVkCommandBuffer(), &copyInfo);

    std::swap(barrier.srcStageMask, barrier.dstStageMask);

    vkCmdPipelineBarrier2(m_cmdBuffer->GetCommandBuffer().GetVkCommandBuffer(), &dependency);
}