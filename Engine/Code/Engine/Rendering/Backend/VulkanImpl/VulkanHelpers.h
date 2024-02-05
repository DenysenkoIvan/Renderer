#pragma once

#include "Framework/Common.h"

#include <vulkan/vulkan.h>

#include "../Buffer.h"
#include "../Texture.h"
#include "../ShaderUtils.h"

static inline BufferUsageFlags BUFFER_USAGE_WRITE_BITS =
    BufferUsageTransferDst |
    BufferUsageStorageWrite;

static inline TextureUsageFlags TEXTURE_USAGE_WRITE_BITS =
    TextureUsageTransferDst |
    TextureUsageColorRenderTarget |
    TextureUsageDepthRenderTarget |
    TextureUsageStorage;

bool BufferUsageHasWrites(BufferUsageFlags usage);

bool TextureUsageHasWrites(TextureUsageFlags usage);

static inline VkAccessFlags2 ACCESS_READ_BITS =
    VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT |
    VK_ACCESS_2_INDEX_READ_BIT |
    VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT |
    VK_ACCESS_2_UNIFORM_READ_BIT |
    VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT |
    VK_ACCESS_2_SHADER_READ_BIT |
    VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    VK_ACCESS_2_TRANSFER_READ_BIT |
    VK_ACCESS_2_HOST_READ_BIT |
    VK_ACCESS_2_MEMORY_READ_BIT |
    VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
    VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

static inline VkAccessFlags2 ACCESS_WRITE_BITS =
    VK_ACCESS_2_SHADER_WRITE_BIT |
    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_2_TRANSFER_WRITE_BIT |
    VK_ACCESS_2_HOST_WRITE_BIT |
    VK_ACCESS_2_MEMORY_WRITE_BIT |
    VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

VkPipelineStageFlags2 BufferUsageToPipelineStages(BufferUsageFlags usage, ShaderStageFlags shaderStages);
VkAccessFlags2 BufferUsageToAccess(BufferUsageFlags usage);

VkPipelineStageFlags2 TextureUsageToPipelineStages(TextureUsageFlags usage);
VkAccessFlags2 TextureUsageToAccess(TextureUsageFlags usage);
VkImageLayout TextureUsageToLayout(TextureUsageFlags usage);

VkPipelineStageFlags2 ShaderStageFlagsToVulkan();