#include "VulkanCommandBuffer.h"

#include "VkContext.h"
#include "VulkanValidation.h"

#include <Framework/Common.h>

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    Assert(m_commandPool && m_commandBuffer);

    vkDestroyCommandPool(VkContext::Get()->GetVkDevice(), m_commandPool, nullptr);
}

void VulkanCommandBuffer::Create()
{
    Assert(!m_commandPool && !m_commandBuffer);

    VkCommandPoolCreateInfo commandPoolCreateInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolCreateInfo.queueFamilyIndex = VkContext::Get()->GetDevice().GetGraphicsQueueIndex();

    VK_VALIDATE(vkCreateCommandPool(VkContext::Get()->GetVkDevice(), &commandPoolCreateInfo, nullptr, &m_commandPool));

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    commandBufferAllocateInfo.commandPool = m_commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VK_VALIDATE(vkAllocateCommandBuffers(VkContext::Get()->GetVkDevice(), &commandBufferAllocateInfo, &m_commandBuffer));

    m_state = CommandBufferState::Initial;
}

void VulkanCommandBuffer::Begin()
{
    Assert(m_state == CommandBufferState::Initial);

    VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    VK_VALIDATE(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));

    m_state = CommandBufferState::Recording;
}

void VulkanCommandBuffer::End()
{
    Assert(m_state == CommandBufferState::Recording);

    VK_VALIDATE(vkEndCommandBuffer(m_commandBuffer));

    m_state = CommandBufferState::Executable;
}

void VulkanCommandBuffer::Reset()
{
    Assert(m_commandPool && m_commandBuffer);

    vkResetCommandPool(VkContext::Get()->GetVkDevice(), m_commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

    m_state = CommandBufferState::Initial;
}

CommandBufferState VulkanCommandBuffer::GetState() const
{
    return m_state;
}

VkCommandBuffer VulkanCommandBuffer::GetVkCommandBuffer() const
{
    Assert(m_commandBuffer);

    return m_commandBuffer;
}