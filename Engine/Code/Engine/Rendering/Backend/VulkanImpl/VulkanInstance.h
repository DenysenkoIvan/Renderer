#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class VulkanInstance
{
public:
    void Create();
    void Destroy();

    VkInstance GetVkInstance();

    const std::vector<const char*>& GetEnabledLayers() const;
    const std::vector<VkValidationFeatureEnableEXT>& GetValidationFeatures() const;
    const std::vector<const char*>& GetExtensions() const;

private:
    void GatherLayers();
    void GatherValidationFeatures();
    void GatherExtensions();
    VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo() const;
    
    void CreateDebugMessenger();

    void DestroyDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    std::vector<const char*> m_layers;
    std::vector<VkValidationFeatureEnableEXT> m_validationFeatures;
    std::vector<const char*> m_extensions;
};