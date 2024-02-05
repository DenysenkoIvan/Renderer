#include "CommandBuffer.h"

#include <Framework/Common.h>

#include "VulkanImpl/VkContext.h"
#include "VulkanImpl/VulkanHelpers.h"
#include "VulkanImpl/VulkanValidation.h"

#include "PSOCompute.h"

std::vector<CommandBufferPtr> CommandBuffer::s_commandBuffersPool;
#if defined(GPU_PROFILE_ENABLE)
GPUQueryPoolTimestamp* CommandBuffer::s_queryPoolTimestamp = nullptr;
#endif

RenderPassState::RenderPassState()
{
    for (auto& attachmentInfo : attachmentInfos)
    {
        attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    }
    depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    Clear();
}

void RenderPassState::SetRenderTarget(int slot, TexturePtr renderTarget, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, const glm::vec4& clearColor, int baseLayer)
{
    if (!renderTarget)
    {
        Error("No render target specified to bind");
        return;
    }

    renderTargets[slot] = renderTarget;
    attachmentsUsed |= 1 << slot;

    VkRenderingAttachmentInfo& colorAttachment = attachmentInfos[slot];
    colorAttachment.imageView = renderTarget->GetTexture().GetView2D(baseLayer, 0, 1);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = loadOp;
    colorAttachment.storeOp = storeOp;
    colorAttachment.clearValue.color.float32[0] = clearColor.r;
    colorAttachment.clearValue.color.float32[1] = clearColor.g;
    colorAttachment.clearValue.color.float32[2] = clearColor.b;
    colorAttachment.clearValue.color.float32[3] = clearColor.a;

    VkImageSubresourceRange& subresource = subresources[slot];
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.baseArrayLayer = baseLayer;
    subresource.layerCount = 1;
    subresource.baseMipLevel = 0;
    subresource.levelCount = 1;
}

void RenderPassState::SetDepthTarget(TexturePtr depthTarget, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, float depthClearValue, uint32_t stencilClearValue)
{
    if (!depthTarget)
    {
        Error("No depth target specified to bind");
        return;
    }

    this->depthTarget = depthTarget;

    depthAttachmentInfo.imageView = depthTarget->GetTexture().GetView2D();
    depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    depthAttachmentInfo.loadOp = loadOp;
    depthAttachmentInfo.storeOp = storeOp;
    depthAttachmentInfo.clearValue.depthStencil.depth = depthClearValue;
    depthAttachmentInfo.clearValue.depthStencil.stencil = stencilClearValue;
}

void RenderPassState::Clear()
{
    attachmentsUsed = 0;

    for (TexturePtr& renderTarget : renderTargets)
    {
        renderTarget = nullptr;
    }
    depthTarget = nullptr;

    VkRenderingAttachmentInfo clearValue{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    clearValue.imageView = VK_NULL_HANDLE;
    clearValue.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    clearValue.resolveMode = VK_RESOLVE_MODE_NONE;
    clearValue.resolveImageView = VK_NULL_HANDLE;
    clearValue.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    clearValue.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    clearValue.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    clearValue.clearValue = {};

    for (VkRenderingAttachmentInfo& info : attachmentInfos)
    {
        info = clearValue;
    }
    depthAttachmentInfo = clearValue;

    for (VkImageSubresourceRange& subresource : subresources)
    {
        subresource.aspectMask = 0;
        subresource.baseArrayLayer = 0;
        subresource.layerCount = 0;
        subresource.baseMipLevel = 0;
        subresource.levelCount = 0;
    }

    isRenderingBegan = false;
}

void BoundResources::Reset()
{
    psoGraphics = nullptr;
    psoCompute = nullptr;

    isPsoGraphicsDirty = true;
    isPsoComputeDirty = true;
}

void BoundResources::SetPsoGraphics(const PSOGraphics* pso)
{
    if (psoGraphics == pso)
    {
        return;
    }

    psoGraphics = pso;
    isPsoGraphicsDirty = true;
    psoCompute = nullptr;
    isPsoComputeDirty = true;
}

bool BoundResources::HasPsoGraphics()
{
    return psoGraphics != nullptr;
}

void BoundResources::SetPsoCompute(const PSOCompute* pso)
{
    if (psoCompute == pso)
    {
        return;
    }

    psoCompute = pso;
    isPsoComputeDirty = true;
    psoGraphics = nullptr;
    isPsoComputeDirty = true;
}

bool BoundResources::HasPsoCompute()
{
    return psoCompute != nullptr;
}

void ResourcesStates::Reset()
{
    buffersUsages.clear();
    texturesUsages.clear();
}

bool ResourcesStates::Empty()
{
    return buffersUsages.empty() && texturesUsages.empty();
}

GPUTimestamp::GPUTimestamp(const char* name, int query)
    : name(name), query(query) {}

bool GPUZone::operator==(const std::string& other) const
{
    return name == other;
}

void CmdBufferStatistics::Create()
{
    pipeline.Create();
}

void CmdBufferStatistics::Destroy()
{
    pipeline.Destroy();
    timestamps.clear();
}

void CmdBufferStatistics::Reset()
{
    pipeline.Reset();
    timestamps.clear();
}

CommandBuffer::~CommandBuffer()
{
    Destroy();
}

void CommandBuffer::Begin()
{
    ProfileFunction();

    m_commandBuffer.Begin();

#if defined(GPU_PROFILE_ENABLE)
    vkCmdResetQueryPool(m_commandBuffer.GetVkCommandBuffer(), m_stats.pipeline.GetVkQueryPool(), 0, 1);
    vkCmdBeginQuery(m_commandBuffer.GetVkCommandBuffer(), m_stats.pipeline.GetVkQueryPool(), 0, 0);
#endif
}

void CommandBuffer::End()
{
    ProfileFunction();

    TryEndRendering();

    if (m_name != "")
    {
        MarkerEnd();
    }

#if defined(GPU_PROFILE_ENABLE)
    vkCmdEndQuery(m_commandBuffer.GetVkCommandBuffer(), m_stats.pipeline.GetVkQueryPool(), 0);
#endif

    m_commandBuffer.End();
}

void CommandBuffer::Reset()
{
    ProfileFunction();

    ResetBindAndRenderStates();
    m_renderPassState.Clear();

    m_commandBuffer.Reset();

    m_stagingBuffers.clear();
    m_resStates.Reset();
    m_boundRes.Reset();

    m_pipelineLayout = VK_NULL_HANDLE;

#if defined(GPU_PROFILE_ENABLE)
    m_stats.Reset();
#endif
}

void CommandBuffer::SetRenderTarget(int slot, TexturePtr renderTarget, bool isPreserveContent)
{
    Assert(renderTarget.get());
    Assert(slot < MAX_RENDER_TARGETS_COUNT);

    ValidateIsInRecordingState();

    if (m_renderPassState.isRenderingBegan)
    {
        Error(false, "Rendering already began, cannot clear render target");
        return;
    }
    VkAttachmentLoadOp loadOp = isPreserveContent ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_renderPassState.SetRenderTarget(slot, renderTarget, loadOp);
}

void CommandBuffer::SetRenderTargetClear(int slot, TexturePtr renderTarget, const glm::vec4& clearColor)
{
    Assert(renderTarget.get());
    Assert(slot < MAX_RENDER_TARGETS_COUNT);

    ValidateIsInRecordingState();

    if (m_renderPassState.isRenderingBegan)
    {
        Error(false, "Rendering already began, cannot bind render target");
        return;
    }

    m_renderPassState.SetRenderTarget(slot, renderTarget, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clearColor);
}

void CommandBuffer::SetRenderTargetLayer(int slot, int layer, TexturePtr renderTarget, bool isPreserveContent)
{
    Assert(renderTarget.get());
    Assert(slot < MAX_RENDER_TARGETS_COUNT);

    ValidateIsInRecordingState();

    if (m_renderPassState.isRenderingBegan)
    {
        Error(false, "Rendering already began, cannot clear render target");
        return;
    }

    VkAttachmentLoadOp loadOp = isPreserveContent ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_renderPassState.SetRenderTarget(slot, renderTarget, loadOp, VK_ATTACHMENT_STORE_OP_STORE, glm::vec4(0.0f), layer);
}

void CommandBuffer::SetDepthTarget(TexturePtr depthTarget, bool isPreserveContent)
{
    Assert(depthTarget.get());

    ValidateIsInRecordingState();

    if (m_renderPassState.isRenderingBegan)
    {
        Error(false, "Rendering already began, cannot bind depth target");
        return;
    }

    VkAttachmentLoadOp loadOp = isPreserveContent ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_renderPassState.SetDepthTarget(depthTarget, loadOp);
}

void CommandBuffer::SetDepthTargetClear(TexturePtr depthTarget, float depthClearValue, uint32_t stencilClearValue)
{
    Assert(depthTarget.get());

    ValidateIsInRecordingState();

    if (m_renderPassState.isRenderingBegan)
    {
        Error(false, "Rendering already began, cannot bind depth target");
        return;
    }

    m_renderPassState.SetDepthTarget(depthTarget, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, depthClearValue, stencilClearValue);
}

void CommandBuffer::PropagateRenderingInfo(PipelineGraphicsState& state)
{
    PipelineRenderingState& renderingState = state.GetRenderingState();

    int colorFormatsCount = 0;

    for (const TexturePtr& renderTarget : m_renderPassState.renderTargets)
    {
        if (!renderTarget)
        {
            continue;
        }

        renderingState.formats[colorFormatsCount] = renderTarget->GetTexture().GetFormat();
        colorFormatsCount++;
    }
    renderingState.state.colorAttachmentCount = colorFormatsCount;
    if (m_renderPassState.depthTarget)
    {
        renderingState.state.depthAttachmentFormat = m_renderPassState.depthTarget->GetTexture().GetFormat();
    }
    else
    {
        renderingState.state.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    }

    PipelineBlendState& blendState = state.GetBlendState();
    blendState.state.attachmentCount = colorFormatsCount;
}

void CommandBuffer::BeginRenderPass()
{
    ValidateIsInRecordingState();

    TryBeginRendering();
    m_stateCompute.Reset();
}

void CommandBuffer::EndRenderPass()
{
    ValidateIsInRecordingState();

    TryEndRendering();
    m_renderPassState.Clear();
}

void CommandBuffer::BindPsoGraphics(const PSOGraphics* pso)
{
    m_boundRes.SetPsoGraphics(pso);
}

void CommandBuffer::BindPsoCompute(const PSOCompute* pso)
{
    m_boundRes.SetPsoCompute(pso);
}

void CommandBuffer::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    m_stateGraphicsDynamic.viewport.x = x;
    m_stateGraphicsDynamic.viewport.y = y;
    m_stateGraphicsDynamic.viewport.width = width;
    m_stateGraphicsDynamic.viewport.height = height;
    m_stateGraphicsDynamic.viewport.minDepth = minDepth;
    m_stateGraphicsDynamic.viewport.maxDepth = maxDepth;
}

void CommandBuffer::SetScissor(int offsetX, int offsetY, int width, int height)
{
    m_stateGraphicsDynamic.scissor.offset.x = offsetX;
    m_stateGraphicsDynamic.scissor.offset.y = offsetY;
    m_stateGraphicsDynamic.scissor.extent.width = width;
    m_stateGraphicsDynamic.scissor.extent.height = height;
}

void CommandBuffer::PushConstants(const void* data, size_t size)
{
    Assert(m_pipelineLayout && size <= 64);

    ProfileFunction();

    ValidateIsInRecordingState();

    vkCmdPushConstants(m_commandBuffer.GetVkCommandBuffer(), m_pipelineLayout, VK_SHADER_STAGE_ALL, 0, (uint32_t)size, data);
}

void CommandBuffer::Draw(uint32_t vtxCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    ProfileFunction();

    ValidateIsInRecordingState();

    Assert(m_renderPassState.isRenderingBegan, "Begin render pass first");

    if (!m_boundRes.HasPsoGraphics())
    {
        LogError("Graphics PSO must be bound");
        return;
    }

    if (m_boundRes.isPsoGraphicsDirty)
    {
        ProfileScope("Binding Graphics Pipeline");
        vkCmdBindPipeline(m_commandBuffer.GetVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_boundRes.psoGraphics->GetPipeline());
        m_boundRes.isPsoGraphicsDirty = false;
    }

    SetDynamicStates();

    vkCmdDraw(m_commandBuffer.GetVkCommandBuffer(), vtxCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::DrawMeshTasks(uint32_t x, uint32_t y, uint32_t z)
{
    ProfileFunction();

    ValidateIsInRecordingState();

    Assert(m_renderPassState.isRenderingBegan, "Begin render pass first");

    if (!m_boundRes.HasPsoGraphics())
    {
        LogError("Graphics PSO must be bound");
        return;
    }

    if (m_boundRes.isPsoGraphicsDirty)
    {
        vkCmdBindPipeline(m_commandBuffer.GetVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_boundRes.psoGraphics->GetPipeline());
        m_boundRes.isPsoGraphicsDirty = false;
    }

    SetDynamicStates();

    VkContext::Get()->GetProcAddresses().vkCmdDrawMeshTasks(m_commandBuffer.GetVkCommandBuffer(), x, y, z);
}

void CommandBuffer::Dispatch(int x, int y, int z)
{
    ProfileFunction();

    ValidateIsInRecordingState();

    if (!m_boundRes.HasPsoCompute())
    {
        LogError("Compute PSO must be bound");
        return;
    }

    if (m_boundRes.isPsoComputeDirty)
    {
        ProfileScope("Bind compute pipeline");

        vkCmdBindPipeline(m_commandBuffer.GetVkCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_boundRes.psoCompute->GetPipeline());
    }

    vkCmdDispatch(m_commandBuffer.GetVkCommandBuffer(), x, y, z);
}

void CommandBuffer::ResetBindAndRenderStates()
{
    m_boundRes.Reset();
    m_stateGraphicsDynamic.Reset();
}

void CommandBuffer::CopyToBuffer(BufferPtr buffer, const void* data, size_t size)
{
    Assert(buffer.get() && data && size);

    ValidateIsInRecordingState();

    AddBufferUsage(buffer, BufferUsageTransferDst);

    BufferPtr stagingBuffer = GetStagingBuffer(size);

    void* stagingBufferPtr = stagingBuffer->Map();
    memcpy(stagingBufferPtr, data, size);
    stagingBuffer->Unmap();

    VkBufferCopy2 region{ .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2 };
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = size;

    VkCopyBufferInfo2 copyInfo{ .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2 };
    copyInfo.srcBuffer = stagingBuffer->GetBuffer().GetVkBuffer();
    copyInfo.dstBuffer = buffer->GetBuffer().GetVkBuffer();
    copyInfo.regionCount = 1;
    copyInfo.pRegions = &region;

    vkCmdCopyBuffer2(m_commandBuffer.GetVkCommandBuffer(), &copyInfo);
}

void CommandBuffer::CopyToTexture(TexturePtr texture, const void* ptr, size_t sizeBytes)
{
    Assert(texture.get());

    ValidateIsInRecordingState();

    VulkanTexture& vulkanTexture = texture->GetTexture();

    BufferPtr stagingBuffer = GetStagingBuffer(sizeBytes);

    void* stagingBufferPtr = stagingBuffer->Map();
    memcpy(stagingBufferPtr, ptr, sizeBytes);
    stagingBuffer->Unmap();

    VkBufferImageCopy2 region{ .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2 };
    region.imageSubresource.aspectMask = vulkanTexture.GetAspect();
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = vulkanTexture.GetExtent3D();

    VkCopyBufferToImageInfo2 copyInfo{ .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2 };
    copyInfo.srcBuffer = stagingBuffer->GetBuffer().GetVkBuffer();
    copyInfo.dstImage = vulkanTexture.GetVkImage();
    copyInfo.dstImageLayout = TextureUsageToLayout(TextureUsageTransferDst);
    copyInfo.regionCount = 1;
    copyInfo.pRegions = &region;

    AddTextureUsage(texture, TextureUsageTransferDst);

    vkCmdCopyBufferToImage2(m_commandBuffer.GetVkCommandBuffer(), &copyInfo);
}

void CommandBuffer::GenerateMipmaps(TexturePtr texture)
{
    AddTextureUsage(texture, TextureUsageTransferDst);

    int mipCount = texture->GetMipCount();

    VkImage vkImage = texture->GetTexture().GetVkImage();

    VkImageMemoryBarrier barrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = vkImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = texture->GetLayerCount();
    barrier.subresourceRange.levelCount = 1;

    int mipWidth = texture->GetWidth();
    int mipHeight = texture->GetHeight();

    VkCommandBuffer vkCmdBuffer = m_commandBuffer.GetVkCommandBuffer();
    for (int i = 1; i < mipCount; i++)
    {
        int mip = i - 1;

        barrier.subresourceRange.baseMipLevel = mip;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(vkCmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = texture->GetLayerCount();
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = texture->GetLayerCount();

        vkCmdBlitImage(vkCmdBuffer,
            vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipCount - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(vkCmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipCount;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(vkCmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

void CommandBuffer::RegisterSRVUsageBuffer(BufferPtr buffer)
{
    if (!buffer)
    {
        return;
    }

    AddBufferUsage(buffer, BufferUsageStorageRead, ShaderStageAll);
}

void CommandBuffer::RegisterSRVUsageTexture(TexturePtr texture)
{
    if (!texture)
    {
        return;
    }

    AddTextureUsage(texture, TextureUsageSample);
}

void CommandBuffer::RegisterUAVUsageBuffer(BufferPtr buffer)
{
    if (!buffer)
    {
        return;
    }

    AddBufferUsage(buffer, BufferUsageStorageRead | BufferUsageStorageWrite, ShaderStageAll);
}

void CommandBuffer::RegisterUAVUsageTexture(TexturePtr texture)
{
    if (!texture)
    {
        return;
    }

    AddTextureUsage(texture, TextureUsageStorage);
}

void CommandBuffer::MarkerBegin(const char* markerName)
{
    VkContextProcAddresses& addresses = VkContext::Get()->GetProcAddresses();
    if (!addresses.vkCmdBeginDebugUtilsLabel || !addresses.vkCmdEndDebugUtilsLabel)
    {
        return;
    }

    VkDebugUtilsLabelEXT labelInfo{ .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    labelInfo.pLabelName = markerName;
    labelInfo.color[1] = 1.0f;
    labelInfo.color[3] = 1.0f;

    addresses.vkCmdBeginDebugUtilsLabel(m_commandBuffer.GetVkCommandBuffer(), &labelInfo);
}

void CommandBuffer::MarkerEnd()
{
    VkContextProcAddresses& addresses = VkContext::Get()->GetProcAddresses();
    if (!addresses.vkCmdBeginDebugUtilsLabel || !addresses.vkCmdEndDebugUtilsLabel)
    {
        return;
    }

    addresses.vkCmdEndDebugUtilsLabel(m_commandBuffer.GetVkCommandBuffer());
}

void CommandBuffer::BeginZone(const char* name)
{
#if defined(GPU_PROFILE_ENABLE)
    int query = s_queryPoolTimestamp->GetNewQuery();

    VkQueryPool pool = s_queryPoolTimestamp->GetVkQueryPool();

    vkCmdWriteTimestamp(m_commandBuffer.GetVkCommandBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, pool, query);

    m_stats.timestamps.emplace_back(name, query);
#endif
}

void CommandBuffer::EndZone()
{
#if defined(GPU_PROFILE_ENABLE)
    int query = s_queryPoolTimestamp->GetNewQuery();

    VkQueryPool pool = s_queryPoolTimestamp->GetVkQueryPool();

    vkCmdWriteTimestamp(m_commandBuffer.GetVkCommandBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, pool, query);

    m_stats.timestamps.emplace_back("", query);
#endif
}

VulkanCommandBuffer& CommandBuffer::GetCommandBuffer()
{
    return m_commandBuffer;
}

CommandBufferPtr CommandBuffer::Create(const char* labelName)
{
    ProfileFunction();

    for (CommandBufferPtr& buffer : s_commandBuffersPool)
    {
        if (buffer.use_count() == 1)
        {
            if (buffer->m_commandBuffer.GetState() != CommandBufferState::Initial)
            {
                buffer->Reset();
            }

            buffer->Begin();

            if (labelName)
            {
                buffer->m_name = labelName;
                buffer->MarkerBegin(labelName);
            }

            return buffer;
        }
    }

    CommandBufferPtr newBuffer = std::make_shared<CommandBuffer>();

    newBuffer->Init();
    newBuffer->Begin();

    if (labelName)
    {
        newBuffer->m_name = labelName;
        newBuffer->MarkerBegin(labelName);
    }

    s_commandBuffersPool.push_back(newBuffer);

    return newBuffer;
}

void CommandBuffer::DestroyPool()
{
    s_commandBuffersPool.clear();
}

void CommandBuffer::SetGPUQueryPoolTimestamp(GPUQueryPoolTimestamp* pool)
{
#if defined(GPU_PROFILE_ENABLE)
    s_queryPoolTimestamp = pool;
#endif
}

void CommandBuffer::Init()
{
    ProfileFunction();

    m_commandBuffer.Create();

#if defined(GPU_PROFILE_ENABLE)
    m_stats.Create();
#endif
}

void CommandBuffer::Destroy()
{
    ProfileFunction();

    Reset();

#if defined(GPU_PROFILE_ENABLE)
    m_stats.Destroy();
#endif
}

void CommandBuffer::TryBeginRendering()
{
    if (!m_renderPassState.isRenderingBegan && (m_renderPassState.attachmentsUsed || m_renderPassState.depthTarget))
    {
        BeginRendering();
        m_renderPassState.isRenderingBegan = true;
    }
}

void CommandBuffer::BeginRendering()
{
    ProfileFunction();

    ValidateIsInRecordingState();

    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    for (int i = 0; i < MAX_RENDER_TARGETS_COUNT; i++)
    {
        if (m_renderPassState.attachmentsUsed & (1 << i))
        {
            colorAttachments.push_back(m_renderPassState.attachmentInfos[i]);

            const VkImageSubresourceRange& subresource = m_renderPassState.subresources[i];

            AddTextureUsage(m_renderPassState.renderTargets[i], TextureUsageColorRenderTarget);
        }
    }

    VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea = GetRenderArea();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = (uint32_t)colorAttachments.size();
    renderingInfo.pColorAttachments = colorAttachments.data();
    if (m_renderPassState.depthTarget)
    {
        renderingInfo.pDepthAttachment = &m_renderPassState.depthAttachmentInfo;

        AddTextureUsage(m_renderPassState.depthTarget, TextureUsageDepthRenderTarget);
    }

    vkCmdBeginRendering(m_commandBuffer.GetVkCommandBuffer(), &renderingInfo);
}

void CommandBuffer::TryEndRendering()
{
    Assert(m_commandBuffer.GetState() == CommandBufferState::Recording);

    ValidateIsInRecordingState();
    
    if (m_renderPassState.isRenderingBegan)
    {
        vkCmdEndRendering(m_commandBuffer.GetVkCommandBuffer());
        m_renderPassState.isRenderingBegan = false;
    }
}

void CommandBuffer::SetDynamicStates()
{
    VkCommandBuffer cmdBuffer = m_commandBuffer.GetVkCommandBuffer();

    vkCmdSetViewportWithCount(cmdBuffer, 1, &m_stateGraphicsDynamic.viewport);
    vkCmdSetScissorWithCount(cmdBuffer, 1, &m_stateGraphicsDynamic.scissor);
}

VkRect2D CommandBuffer::GetRenderArea() const
{
    Assert(m_renderPassState.attachmentsUsed || m_renderPassState.depthTarget);

    TexturePtr first;
    for (const TexturePtr& renderTarget : m_renderPassState.renderTargets)
    {
        if (!renderTarget)
        {
            continue;
        }

        if (!first)
        {
            first = renderTarget;
        }

        if (renderTarget->GetWidth() != first->GetWidth() || renderTarget->GetHeight() != first->GetHeight())
        {
            LogError("Render targets have different dimensions");
        }
    }

    TexturePtr depthTarget = m_renderPassState.depthTarget;
    if (depthTarget)
    {
        if (first)
        {
            if (depthTarget->GetWidth() != first->GetWidth() || depthTarget->GetHeight() != first->GetHeight())
            {
                LogError("Depth target has different dimensions");
            }
        }
        else
        {
            first = depthTarget;
        }
    }

    if (!first)
    {
        first = m_renderPassState.depthTarget;
    }

    VkRect2D renderArea{};
    renderArea.offset = { 0, 0 };
    renderArea.extent = { (uint32_t)first->GetWidth(), (uint32_t)first->GetHeight() };

    return renderArea;
}

BufferPtr CommandBuffer::GetStagingBuffer(size_t sizeBytes)
{
    m_stagingBuffers.push_back(Buffer::CreateStaging(sizeBytes));

    return m_stagingBuffers.back();
}

void CommandBuffer::AddBufferUsage(const BufferPtr& buffer, BufferUsageFlags newUsage, ShaderStageFlags newStages)
{
    auto bufferUsagesIt = m_resStates.buffersUsages.find(buffer);
    if (bufferUsagesIt == m_resStates.buffersUsages.end())
    {
        BufferState state(newUsage, newStages);

        BufferStates& bufferStates = m_resStates.buffersUsages[buffer] = {};
        bufferStates.initial = state;
        bufferStates.last = state;
    }
    else
    {
        BufferStates& bufferStates = bufferUsagesIt->second;
        BufferState& lastState = bufferStates.last;
        BufferState newState(newUsage, newStages);

        if (lastState.IsUndefined() || BufferUsageHasWrites(lastState.usage) || BufferUsageHasWrites(newUsage))
        {
            Assert(!m_renderPassState.isRenderingBegan);
            AddBufferBarrier(buffer, lastState, newState);
        }

        lastState = newState;
    }
}

void CommandBuffer::AddTextureUsage(const TexturePtr& texture, TextureUsageFlags newUsage)
{   
    auto textureUsagesIt = m_resStates.texturesUsages.find(texture);
    if (textureUsagesIt == m_resStates.texturesUsages.end())
    {
        TextureState state(newUsage);

        TextureStates& textureStates = m_resStates.texturesUsages[texture] = {};
        textureStates.initial = state;
        textureStates.last = state;
    }
    else
    {
        TextureStates& textureStates = textureUsagesIt->second;
        TextureState& lastState = textureStates.last;
        TextureState newState(newUsage);

        if (lastState.IsUndefined() || TextureUsageHasWrites(lastState.usage) || TextureUsageHasWrites(newUsage))
        {
            Assert(!m_renderPassState.isRenderingBegan);
            AddTextureBarrier(texture, lastState, newState);
        }

        lastState = newState;
    }
}

void CommandBuffer::AddBufferBarrier(const BufferPtr& buffer, const BufferState& prevState, const BufferState& newState)
{
    ValidateIsInRecordingState();

    VkBufferMemoryBarrier2 bufferBarrier{ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
    bufferBarrier.srcStageMask = BufferUsageToPipelineStages(prevState.usage, prevState.stages);
    bufferBarrier.dstStageMask = BufferUsageToPipelineStages(newState.usage, newState.stages);
    bufferBarrier.srcAccessMask = BufferUsageToAccess(prevState.usage);
    bufferBarrier.dstAccessMask = BufferUsageToAccess(newState.usage);
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.buffer = buffer->GetBuffer().GetVkBuffer();
    bufferBarrier.offset = 0;
    bufferBarrier.size = buffer->GetSize();

    VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.bufferMemoryBarrierCount = 1;
    dependencyInfo.pBufferMemoryBarriers = &bufferBarrier;

    vkCmdPipelineBarrier2(m_commandBuffer.GetVkCommandBuffer(), &dependencyInfo);
}

void CommandBuffer::AddTextureBarrier(const TexturePtr& texture, const TextureState& prevState, const TextureState& newState)
{
    ValidateIsInRecordingState();

    Assert(!m_renderPassState.isRenderingBegan);

    VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    imageBarrier.srcStageMask = TextureUsageToPipelineStages(prevState.usage);
    imageBarrier.dstStageMask = TextureUsageToPipelineStages(newState.usage);
    imageBarrier.srcAccessMask = TextureUsageToAccess(prevState.usage);
    imageBarrier.dstAccessMask = TextureUsageToAccess(newState.usage);
    imageBarrier.oldLayout = TextureUsageToLayout(prevState.usage);
    imageBarrier.newLayout = TextureUsageToLayout(newState.usage);
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = texture->GetTexture().GetVkImage();
    imageBarrier.subresourceRange.aspectMask = texture->GetTexture().GetAspect();
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = texture->GetMipCount();
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = texture->GetLayerCount();

    VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(m_commandBuffer.GetVkCommandBuffer(), &dependencyInfo);
}

void CommandBuffer::ValidateIsInRecordingState()
{
    Assert(m_commandBuffer.GetState() == CommandBufferState::Recording);
}