#pragma once

#include <vulkan/vulkan.h>

#include <memory>

enum class CommandBufferState : uint8_t
{
    Initial,
    Recording,
    Executable,
    Pending,
    Invalid
};

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer() = default;
    ~VulkanCommandBuffer();
    
    void Create();

    void Begin();
    void End();

    void Reset();

    CommandBufferState GetState() const;
    VkCommandBuffer GetVkCommandBuffer() const;

private:
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    CommandBufferState m_state;
};