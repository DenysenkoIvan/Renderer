#include "VulkanSurface.h"

#include "VkContext.h"
#include "VulkanValidation.h"

void VulkanSurface::Create(VkInstance instance, VkPhysicalDevice physicalDevice)
{
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = Window::Get()->GetWindowHandle();
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

    VK_VALIDATE(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &m_surface));

    uint32_t formatCount = 0;
    VK_VALIDATE(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, nullptr));
    m_formats.resize(formatCount);
    VK_VALIDATE(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, m_formats.data()));

    uint32_t presentModeCount = 0;
    VK_VALIDATE(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, nullptr));
    m_presentModes.resize(presentModeCount);
    VK_VALIDATE(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, m_presentModes.data()));
}

void VulkanSurface::Destroy()
{
    Assert(m_surface, "Vulkan surface should be created to be destroyed");

    vkDestroySurfaceKHR(VkContext::Get()->GetVkInstance(), m_surface, nullptr);
}

VkSurfaceKHR VulkanSurface::GetVkSurface() const
{
    Error(m_surface, "Vulkan Surface is invalid");

    return m_surface;
}

VkSurfaceCapabilitiesKHR VulkanSurface::GetCapabilities() const
{
    Assert(m_surface);
    
    VkSurfaceCapabilitiesKHR capabilities{};

    VK_VALIDATE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkContext::Get()->GetVkPhysicalDevice(), m_surface, &capabilities));

    return capabilities;
}

const std::vector<VkSurfaceFormatKHR>& VulkanSurface::GetFormats() const
{
    Assert(m_surface);

    return m_formats;
}

const std::vector<VkPresentModeKHR>& VulkanSurface::GetPresentModes() const
{
    Assert(m_surface);

    return m_presentModes;
}