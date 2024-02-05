#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "VMA.h"
#include "VulkanCommandBuffer.h"

class VulkanSwapchain;

namespace
{
    struct ImageViewRegion
    {
        VkImageViewCreateInfo info{};
        VkImageView view = VK_NULL_HANDLE;

        ImageViewRegion()
        {
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        }
    };
}

class VulkanTexture
{
    friend class VulkanSwapchain;

public:
    VulkanTexture() = default;
    ~VulkanTexture();

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    void Create(VkImageType type, VkFormat format, VkExtent3D extent, uint32_t mipCount, uint32_t layerCount,
        VkSampleCountFlagBits sampleCount, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode,
        uint32_t queueFamilyIndexCount, const uint32_t* queueFamilyIndices, VkImageLayout initialLayout);
    void Create(VkImage image, VkImageType type, VkFormat format, VkExtent3D extent, uint32_t mipCount, uint32_t layerCount,
        VkSampleCountFlagBits sampleCount, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode);

    VkImage GetVkImage() const { return m_image; }
    VkImageType GetType() const { return m_imageType; }
    VkFormat GetFormat() const { return m_format; }
    VkExtent3D GetExtent3D() const { return m_extent; }
    int GetMipCount() const { return m_mipCount; }
    int GetLayerCount() const { return m_layerCount; }
    VkImageTiling GetTiling() const { return m_tiling; }
    VkImageUsageFlags GetUsage() const { return m_usage; }
    VkImageAspectFlags GetAspect() const { return m_aspect; }

    VkImageView GetView2D(int baseLayer = 0, int baseMip = 0, int mipCount = 1);
    VkImageView GetViewCube(int baseMip = 0, int mipCount = 1);

private:
    void Destroy();

    static VkImage CreateVkImage(VkImageType type, VkFormat format, VkExtent3D extent, uint32_t mipCount, uint32_t layerCount,
        VkSampleCountFlagBits sampleCount, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode,
        uint32_t queueFamilyIndexCount, const uint32_t* queueFamilyIndices, VkImageLayout initialLayout);

    void DefineAspect();

private:
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_vmaAllocation = VK_NULL_HANDLE;

    VkImageType m_imageType = VK_IMAGE_TYPE_2D;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent3D m_extent{ 1, 1, 1 };
    int m_mipCount = 1;
    int m_layerCount = 1;
    VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling m_tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags m_usage = 0;
    VkImageAspectFlags m_aspect = VK_IMAGE_ASPECT_NONE;
    VkSharingMode m_sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    std::vector<ImageViewRegion> m_views;
};