#include "VulkanHelpers.h"

bool BufferUsageHasWrites(BufferUsageFlags usage)
{
    return (usage & BUFFER_USAGE_WRITE_BITS) != 0;
}

bool TextureUsageHasWrites(TextureUsageFlags usage)
{
    return (usage & TEXTURE_USAGE_WRITE_BITS) != 0;
}

VkPipelineStageFlags2 BufferUsageToPipelineStages(BufferUsageFlags usage, ShaderStageFlags shaderStages)
{
    if (usage == 0)
    {
        return VK_PIPELINE_STAGE_2_NONE;
    }

    VkPipelineStageFlags2 stages = 0;

    if (usage & BufferUsageTransferSrc)
    {
        stages |= VK_PIPELINE_STAGE_2_COPY_BIT;
    }
    if (usage & BufferUsageTransferDst)
    {
        stages |= VK_PIPELINE_STAGE_2_COPY_BIT;
    }
    if (usage & BufferUsageStorageRead)
    {
        stages |= ShaderStageFlagsToVulkan();
    }
    if (usage & BufferUsageStorageWrite)
    {
        stages |= ShaderStageFlagsToVulkan();
    }

    return stages;
}

VkAccessFlags2 BufferUsageToAccess(BufferUsageFlags usage)
{
    if (usage == 0)
    {
        return VK_ACCESS_2_NONE;
    }

    VkAccessFlags2 access = 0;

    if (usage & BufferUsageTransferSrc)
    {
        access |= VK_ACCESS_2_TRANSFER_READ_BIT;
    }
    if (usage & BufferUsageTransferDst)
    {
        access |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    if (usage & BufferUsageStorageRead)
    {
        access |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
    }
    if (usage & BufferUsageStorageWrite)
    {
        access |= VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
    }

    return access;
}

VkPipelineStageFlags2 TextureUsageToPipelineStages(TextureUsageFlags usage)
{
    if (usage == 0)
    {
        return VK_PIPELINE_STAGE_2_NONE;
    }

    VkPipelineStageFlags2 stages = 0;

    if (usage & TextureUsageTransferSrc)
    {
        stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    }
    if (usage & TextureUsageTransferDst)
    {
        stages |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    }
    if (usage & TextureUsageSample)
    {
        stages |= ShaderStageFlagsToVulkan();
    }
    if (usage & TextureUsageColorRenderTarget)
    {
        stages |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT;
    }
    if (usage & TextureUsageDepthRenderTarget)
    {
        stages |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_RESOLVE_BIT;
    }
    if (usage & TextureUsageStorage)
    {
        stages |= ShaderStageFlagsToVulkan();
    }

    return stages;
}

VkAccessFlags2 TextureUsageToAccess(TextureUsageFlags usage)
{
    if (usage == 0)
    {
        return VK_ACCESS_2_NONE;
    }

    VkAccessFlags2 access = 0;

    if (usage & TextureUsageTransferSrc)
    {
        access |= VK_ACCESS_2_TRANSFER_READ_BIT;
    }
    if (usage & TextureUsageTransferDst)
    {
        access |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    if (usage & TextureUsageSample)
    {
        access |= VK_ACCESS_2_SHADER_READ_BIT;
    }
    if (usage & TextureUsageColorRenderTarget)
    {
        access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (usage & TextureUsageDepthRenderTarget)
    {
        access |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if (usage & TextureUsageStorage)
    {
        return VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
    }

    return access;
}

VkImageLayout TextureUsageToLayout(TextureUsageFlags usage)
{
    if (usage == 0)
    {
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    Assert(std::popcount(usage) == 1);

    if (usage & TextureUsageTransferSrc)
    {
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
    if (usage & TextureUsageTransferDst)
    {
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }
    if (usage & TextureUsageSample)
    {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    if (usage & TextureUsageColorRenderTarget)
    {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (usage & TextureUsageDepthRenderTarget)
    {
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    if (usage & TextureUsageStorage)
    {
        return VK_IMAGE_LAYOUT_GENERAL;
    }
    if (usage & TextureUsagePresent)
    {
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkPipelineStageFlags2 ShaderStageFlagsToVulkan()
{
    return VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT |
        VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT |
        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
}