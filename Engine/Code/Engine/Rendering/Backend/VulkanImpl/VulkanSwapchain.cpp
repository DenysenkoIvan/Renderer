#include "VulkanSwapchain.h"

#include "VkContext.h"
#include "VulkanValidation.h"

VulkanSwapchain::~VulkanSwapchain()
{
    Destroy();
}

void VulkanSwapchain::Create()
{
    ChooseFormat();
    ChoosePresentMode();
    ChooseExtent();

    CreateSwapchain();

    RetrieveImages();
}

void VulkanSwapchain::Resize()
{
    ChooseExtent();

    CreateSwapchain();

    if (IsRenderable())
    {
        RetrieveImages();
    }

    m_imageIndex = -1;
}

bool VulkanSwapchain::IsRenderable()
{
    return m_swapchain && m_extent.width && m_extent.height;
}

bool VulkanSwapchain::AcquireNextImage(VkSemaphore semaphore, VkFence fence)
{
    ProfileFunction();

    Assert(IsRenderable());

    VkResult result = vkAcquireNextImageKHR(VkContext::Get()->GetVkDevice(), m_swapchain, 0, semaphore, fence, &m_imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        Resize();
        AcquireNextImage(semaphore, fence);

        return true;
    }

    return false;
}

void VulkanSwapchain::Present(VkSemaphore semaphore)
{
    ProfileStall("Swapchain Present");

    Assert(semaphore);

    VkResult result = VK_ERROR_UNKNOWN;

    VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_imageIndex;
    presentInfo.pResults = &result;

    vkQueuePresentKHR(VkContext::Get()->GetDevice().GetPresentQueue(), &presentInfo);
}

uint32_t VulkanSwapchain::GetImageCount()
{
    return (uint32_t)m_images.size();
}

uint32_t VulkanSwapchain::GetImageIndex()
{
    Assert(m_swapchain && m_imageIndex != -1);

    return m_imageIndex;
}

VkImage VulkanSwapchain::GetImage(uint32_t index)
{
    Assert(m_swapchain && index != -1 && index < m_images.size());

    return m_images[index];
}

VkSurfaceFormatKHR VulkanSwapchain::GetSurfaceFormat() const
{
    Assert(m_swapchain);

    return m_surfaceFormat;
}

VkPresentModeKHR VulkanSwapchain::GetPresentMode() const
{
    Assert(m_swapchain);

    return m_presentMode;
}

VkExtent2D VulkanSwapchain::GetExtent() const
{
    return m_extent;
}

VkImageUsageFlags VulkanSwapchain::GetImageUsage() const
{
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
}

void VulkanSwapchain::CreateSwapchain()
{
    VkSwapchainKHR oldSwapchain = m_swapchain;

    uint32_t imageCount = 3;
    VkSurfaceCapabilitiesKHR capabilities = VkContext::Get()->GetSurface().GetCapabilities();
    
    imageCount = std::min(imageCount, capabilities.maxImageCount);
    imageCount = std::max(imageCount, capabilities.minImageCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfo{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchainCreateInfo.surface = VkContext::Get()->GetSurface().GetVkSurface();
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = m_surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = m_surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = m_extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = GetImageUsage();
    swapchainCreateInfo.oldSwapchain = oldSwapchain;

    uint32_t graphicsQueue = VkContext::Get()->GetDevice().GetGraphicsQueueIndex();
    uint32_t presentQueue = VkContext::Get()->GetDevice().GetPresentQueueIndex();

    if (graphicsQueue == presentQueue)
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        uint32_t queues[2] = { graphicsQueue, presentQueue };
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queues;
    }

    swapchainCreateInfo.preTransform = capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = m_presentMode;
    swapchainCreateInfo.clipped = true;

    VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
    if (m_extent.width != 0 && m_extent.height != 0)
    {
        VK_VALIDATE(vkCreateSwapchainKHR(VkContext::Get()->GetVkDevice(), &swapchainCreateInfo, nullptr, &newSwapchain));
    }
        
    m_swapchain = newSwapchain;

    vkDestroySwapchainKHR(VkContext::Get()->GetVkDevice(), oldSwapchain, nullptr);

    m_images.clear();
}

void VulkanSwapchain::Destroy()
{
    Assert(m_swapchain);

    m_images.clear();

    VkDevice device = VkContext::Get()->GetVkDevice();

    vkDestroySwapchainKHR(device, m_swapchain, nullptr);
}

void VulkanSwapchain::ChooseExtent()
{
    VkSurfaceCapabilitiesKHR capabilities = VkContext::Get()->GetSurface().GetCapabilities();

    m_extent = capabilities.currentExtent;
}

void VulkanSwapchain::ChooseFormat()
{
    const std::vector<VkSurfaceFormatKHR>& availableFormats = VkContext::Get()->GetSurface().GetFormats();

    bool formatFound = false;
    for (const VkSurfaceFormatKHR& format : availableFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            formatFound = true;
            m_surfaceFormat = format;
            break;
        }
    }

    Assert(formatFound);
}

void VulkanSwapchain::ChoosePresentMode()
{
    const std::vector<VkPresentModeKHR>& availablePresentModes = VkContext::Get()->GetSurface().GetPresentModes();

    bool presentModeFound = false;
    for (VkPresentModeKHR presentMode : availablePresentModes)
    {
        if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            presentModeFound = true;
            m_presentMode = presentMode;
            break;
        }
    }

    Assert(presentModeFound);
}

void VulkanSwapchain::RetrieveImages()
{
    VkDevice device = VkContext::Get()->GetVkDevice();

    uint32_t imageCount = 0;
    VK_VALIDATE(vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, nullptr));
    m_images.resize(imageCount);
    VK_VALIDATE(vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, m_images.data()));
}