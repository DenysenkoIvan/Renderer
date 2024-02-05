#pragma once

#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"

class VulkanDevice
{
public:
    void Create(VulkanPhysicalDevice& physicalDevice, VkSurfaceKHR surface);
    void Destroy();

    VkDevice GetVkDevice() const;
    uint32_t GetGraphicsQueueIndex() const;
    uint32_t GetPresentQueueIndex() const;
    VkQueue GetGraphicsQueue() const;
    VkQueue GetPresentQueue() const;
    const VkPhysicalDeviceFeatures& GetEnabledFeatures() const;
    const VkPhysicalDeviceVulkan13Features& GetEnabledFeatures13() const;

    void WaitIdle() const;

private:
    void GatherQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void GatherEnabledFeatures(VulkanPhysicalDevice& physicalDevice);

    VkDeviceQueueCreateInfo CreateQueueCreateInfo(uint32_t queue) const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueIndex = -1;
    uint32_t m_presentQueueIndex = -1;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures2 m_enabledFeatures{};
    VkPhysicalDeviceVulkan11Features m_enabledFeatures11{};
    VkPhysicalDeviceVulkan12Features m_enabledFeatures12{};
    VkPhysicalDeviceVulkan13Features m_enabledFeatures13{};
};