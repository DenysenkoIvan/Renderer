#pragma once

#include "VulkanInstance.h"

class VulkanPhysicalDevice
{
public:
    void Create(VkInstance instance);

    VkPhysicalDevice GetVkPhysicalDevice() const;
    const VkPhysicalDeviceProperties& GetProperties() const;
    const VkPhysicalDeviceDescriptorBufferPropertiesEXT& GetDescriptorBufferProperties() const;
    const VkPhysicalDeviceFeatures& GetFeatures() const;
    const VkPhysicalDeviceVulkan11Features& GetFeatures11() const;
    const VkPhysicalDeviceVulkan12Features& GetFeatures12() const;
    const VkPhysicalDeviceVulkan13Features& GetFeatures13() const;
    const std::vector<const char*>& GetExtensions();

private:
    void GatherExtensions();
    void PickPhysicalDevice(VkInstance instance);
    void RetrieveFeatures();

private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties2 m_properties{};
    VkPhysicalDeviceDescriptorBufferPropertiesEXT m_descriptorBufferProps{};
    VkPhysicalDeviceFeatures m_featuresCore{};
    VkPhysicalDeviceVulkan11Features m_features11{};
    VkPhysicalDeviceVulkan12Features m_features12{};
    VkPhysicalDeviceVulkan13Features m_features13{};
    std::vector<const char*> m_extensions;
};