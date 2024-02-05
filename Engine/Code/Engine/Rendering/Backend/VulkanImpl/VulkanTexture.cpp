#include "VulkanTexture.h"

#include <algorithm>

#include "VkContext.h"
#include "VulkanValidation.h"

namespace
{
    bool HasDepth(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
            return true;
        default:
            return false;
        }
    }

    bool HasStencil(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        default:
            return false;
        }
    }
}

VulkanTexture::~VulkanTexture()
{
    Destroy();
}

void VulkanTexture::Create(VkImageType type, VkFormat format, VkExtent3D extent, uint32_t mipCount, uint32_t layerCount,
    VkSampleCountFlagBits sampleCount, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode,
    uint32_t queueFamilyIndexCount, const uint32_t* queueFamilyIndices, VkImageLayout initialLayout)
{
    m_imageType = type;
    m_format = format;
    m_extent = extent;
    m_mipCount = mipCount;
    m_layerCount = layerCount;
    m_sampleCount = sampleCount;
    m_tiling = tiling;
    m_usage = usage;
    m_sharingMode = sharingMode;

    VkImageCreateInfo imageInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.flags = 0;
    imageInfo.imageType = type;
    imageInfo.format = format;
    imageInfo.extent = extent;
    imageInfo.mipLevels = mipCount;
    imageInfo.arrayLayers = layerCount;
    imageInfo.samples = sampleCount;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.sharingMode = sharingMode;
    imageInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    imageInfo.pQueueFamilyIndices = queueFamilyIndices;
    imageInfo.initialLayout = initialLayout;

    if (layerCount == 6)
    {
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocationInfo info{};

    VK_VALIDATE(vmaCreateImage(VMA::Allocator(), &imageInfo, &allocationCreateInfo, &m_image, &m_vmaAllocation, &info));

    DefineAspect();
}

void VulkanTexture::Create(VkImage image, VkImageType type, VkFormat format, VkExtent3D extent, uint32_t mipCount, uint32_t layerCount,
    VkSampleCountFlagBits sampleCount, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode)
{
    m_image = image;
    m_imageType = type;
    m_format = format;
    m_extent = extent;
    m_mipCount = mipCount;
    m_layerCount = layerCount;
    m_sampleCount = sampleCount;
    m_tiling = tiling;
    m_usage = usage;
    m_sharingMode = sharingMode;

    DefineAspect();
}

VkImageView VulkanTexture::GetView2D(int baseLayer, int baseMip, int mipCount)
{
    Assert(m_image && m_format != VK_FORMAT_UNDEFINED && m_aspect != VK_IMAGE_ASPECT_NONE);

    VkImageAspectFlags aspect = 0;
    if (HasDepth(m_format))
    {
        aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;

        if (HasStencil(m_format))
        {
            aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    ImageViewRegion viewRegion{};
    viewRegion.info.image = m_image;
    viewRegion.info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewRegion.info.format = m_format;
    viewRegion.info.subresourceRange.aspectMask = aspect;
    viewRegion.info.subresourceRange.baseMipLevel = baseMip;
    viewRegion.info.subresourceRange.levelCount = mipCount;
    viewRegion.info.subresourceRange.baseArrayLayer = baseLayer;
    viewRegion.info.subresourceRange.layerCount = 1;

    auto it = std::find_if(m_views.begin(), m_views.end(), [&viewRegion](const ImageViewRegion& region)
    {
        return 0 == memcmp(&viewRegion.info, &region.info, sizeof(VkImageViewCreateInfo));
    });

    if (it != m_views.end())
    {
        return it->view;
    }

    VK_VALIDATE(vkCreateImageView(VkContext::Get()->GetVkDevice(), &viewRegion.info, nullptr, &viewRegion.view));

    m_views.push_back(viewRegion);

    return viewRegion.view;
}

VkImageView VulkanTexture::GetViewCube(int baseMip, int mipCount)
{
    Assert(m_image && m_format != VK_FORMAT_UNDEFINED && m_aspect != VK_IMAGE_ASPECT_NONE && !HasDepth(m_format) && !HasStencil(m_format));

    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    ImageViewRegion viewRegion{};
    viewRegion.info.image = m_image;
    viewRegion.info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewRegion.info.format = m_format;
    viewRegion.info.subresourceRange.aspectMask = aspect;
    viewRegion.info.subresourceRange.baseMipLevel = baseMip;
    viewRegion.info.subresourceRange.levelCount = mipCount;
    viewRegion.info.subresourceRange.baseArrayLayer = 0;
    viewRegion.info.subresourceRange.layerCount = 6;

    auto it = std::find_if(m_views.begin(), m_views.end(), [&viewRegion](const ImageViewRegion& region)
        {
            return 0 == memcmp(&viewRegion.info, &region.info, sizeof(VkImageViewCreateInfo));
        });

    if (it != m_views.end())
    {
        return it->view;
    }

    VK_VALIDATE(vkCreateImageView(VkContext::Get()->GetVkDevice(), &viewRegion.info, nullptr, &viewRegion.view));

    m_views.push_back(viewRegion);

    return viewRegion.view;
}

void VulkanTexture::Destroy()
{
    if (!m_views.empty())
    {
        for (ImageViewRegion& viewRegion : m_views)
        {
            vkDestroyImageView(VkContext::Get()->GetVkDevice(), viewRegion.view, nullptr);
        }

        m_views.clear();
    }

    if (m_vmaAllocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(VMA::Allocator(), m_image, m_vmaAllocation);
    }
    
    m_vmaAllocation = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
}

VkImage VulkanTexture::CreateVkImage(VkImageType type, VkFormat format, VkExtent3D extent, uint32_t mipCount, uint32_t layerCount,
    VkSampleCountFlagBits sampleCount, VkImageTiling tiling, VkImageUsageFlags usage, VkSharingMode sharingMode,
    uint32_t queueFamilyIndexCount, const uint32_t* queueFamilyIndices, VkImageLayout initialLayout)
{
    VkImage image = VK_NULL_HANDLE;

    VkImageCreateInfo createInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    createInfo.imageType = type;
    createInfo.format = format;
    createInfo.extent = extent;
    createInfo.mipLevels = mipCount;
    createInfo.arrayLayers = layerCount;
    createInfo.samples = sampleCount;
    createInfo.tiling = tiling;
    createInfo.usage = usage;
    createInfo.sharingMode = sharingMode;
    createInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
    createInfo.initialLayout = initialLayout;


    VK_VALIDATE(vkCreateImage(VkContext::Get()->GetVkDevice(), &createInfo, nullptr, &image));

    return image;
}

void VulkanTexture::DefineAspect()
{
    bool hasDepth = HasDepth(m_format);
    bool hasStencil = HasStencil(m_format);

    m_aspect = 0;
    if (!hasDepth && !hasStencil)
    {
        m_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else
    {
        if (hasDepth)
        {
            m_aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (hasStencil)
        {
            m_aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
}