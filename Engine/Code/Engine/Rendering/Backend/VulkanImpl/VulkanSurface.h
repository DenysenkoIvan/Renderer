#pragma once

#include <Window/Window.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

class VulkanSurface
{
public:
    void Create(VkInstance instance, VkPhysicalDevice physicalDevice);
    void Destroy();

    VkSurfaceKHR GetVkSurface() const;
    VkSurfaceCapabilitiesKHR GetCapabilities() const;
    const std::vector<VkSurfaceFormatKHR>& GetFormats() const;
    const std::vector<VkPresentModeKHR>& GetPresentModes() const;

private:
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    std::vector<VkSurfaceFormatKHR> m_formats;
    std::vector<VkPresentModeKHR> m_presentModes;
};