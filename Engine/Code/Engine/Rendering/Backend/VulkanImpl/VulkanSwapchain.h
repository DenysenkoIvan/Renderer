#pragma once

#include "VulkanInstance.h"
#include "VulkanTexture.h"

#include <functional>

class VulkanSwapchain
{
public:
    ~VulkanSwapchain();

    void Create();

    void Resize();

    bool IsRenderable();

    bool AcquireNextImage(VkSemaphore semaphore, VkFence fence);
    void Present(VkSemaphore semaphore);

    uint32_t GetImageCount();

    uint32_t GetImageIndex();
    VkImage GetImage(uint32_t index);

    VkSurfaceFormatKHR GetSurfaceFormat() const;
    VkPresentModeKHR GetPresentMode() const;
    VkExtent2D GetExtent() const;

    VkImageUsageFlags GetImageUsage() const;

private:
    void CreateSwapchain();
    void Destroy();

    void ChooseExtent();
    void ChooseFormat();
    void ChoosePresentMode();
    void RetrieveImages();

private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkSurfaceFormatKHR m_surfaceFormat;
    VkPresentModeKHR m_presentMode;
    VkExtent2D m_extent;
    std::vector<VkImage> m_images;
    uint32_t m_imageIndex = -1;
};