#include "RenderDriver.h"

#include <imgui.h>

#include "VulkanImpl/VulkanHelpers.h"
#include "VulkanImpl/VulkanValidation.h"
#include "VulkanImpl/VMA.h"

#include "PSOGraphics.h"
#include "PSOCompute.h"

void Frame::Create()
{
    VkDevice device = VkContext::Get()->GetVkDevice();

    VkSemaphoreCreateInfo semaphoreCreateInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VK_VALIDATE(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailabeSemaphore));
    VK_VALIDATE(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore));

    VkFenceCreateInfo fenceCreateInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_VALIDATE(vkCreateFence(device, &fenceCreateInfo, nullptr, &renderCompleteFence));

#if defined(GPU_PROFILE_ENABLE)
    timestampQueryPool.Create();
#endif
}

void Frame::Destroy()
{
    VkDevice device = VkContext::Get()->GetVkDevice();

    if (renderCompleteFence)
    {
        vkDestroyFence(device, renderCompleteFence, nullptr);
        renderCompleteFence = VK_NULL_HANDLE;
    }

    if (renderFinishedSemaphore)
    {
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        renderFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (imageAvailabeSemaphore)
    {
        vkDestroySemaphore(device, imageAvailabeSemaphore, nullptr);
        imageAvailabeSemaphore = VK_NULL_HANDLE;
    }

#if defined(GPU_PROFILE_ENABLE)
    timestampQueryPool.Destroy();
#endif

    usedCmdBuffers.clear();
}

void Frame::Reset()
{
    for (CommandBufferPtr& cmdBuffer : usedCmdBuffers)
    {
        cmdBuffer->Reset();
    }
    usedCmdBuffers.clear();

#if defined(GPU_PROFILE_ENABLE)
    timestampQueryPool.Reset();
#endif
}

void FrameData::Create()
{
    Assert(frameIndex == -1);

    for (Frame& frame : frames)
    {
        frame.Create();
    }

    frameIndex = 0;
}

void FrameData::Destroy()
{
    for (Frame& frame : frames)
    {
        frame.Destroy();
    }

    frameIndex = -1;
}

void FrameData::CreateSwapchainSyncPrimitives()
{
    VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    for (Frame& frame : frames)
    {
        Assert(!frame.imageAvailabeSemaphore);

        VK_VALIDATE(vkCreateSemaphore(VkContext::Get()->GetVkDevice(), &semaphoreInfo, nullptr, &frame.imageAvailabeSemaphore));
        VK_VALIDATE(vkCreateSemaphore(VkContext::Get()->GetVkDevice(), &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore));
    }
}

void FrameData::DestroySwapchainSyncPrimitives()
{
    for (Frame& frame : frames)
    {
        vkDestroySemaphore(VkContext::Get()->GetVkDevice(), frame.imageAvailabeSemaphore, nullptr);
        vkDestroySemaphore(VkContext::Get()->GetVkDevice(), frame.renderFinishedSemaphore, nullptr);
        frame.imageAvailabeSemaphore = VK_NULL_HANDLE;
        frame.renderFinishedSemaphore = VK_NULL_HANDLE;
    }
}

uint32_t FrameData::GetFrameIndex()
{
    return frameIndex;
}

Frame& FrameData::GetFrame()
{
    Assert(frameIndex != -1);
    
    return frames[frameIndex];
}

void FrameData::NextFrame()
{
    Assert(frameIndex != -1);
    
    frameIndex = (frameIndex + 1) % FRAME_COUNT;
}

RenderDriver::RenderDriver()
{
    m_context.Create();
    VkContext::Set(&m_context);

    VMA::Initialize();

    m_frameData.Create();
}

RenderDriver::~RenderDriver()
{
    m_context.GetDevice().WaitIdle();

    m_frameData.Destroy();

    CommandBuffer::DestroyPool();

    PSOGraphics::DestroyCache();
    PSOCompute::DestroyCache();
    Shader::ClearCache();
    Sampler::DestroySamplers();

    VMA::Deinitialize();

    m_context.Destroy();

    VkContext::Set(nullptr);
}

void RenderDriver::DestroyFramesData()
{
    m_frameData.Destroy();
    CommandBuffer::DestroyPool();
}

void RenderDriver::BeginFrame()
{
    ProfileFunction();

    m_frameData.NextFrame();

    Frame& frame = m_frameData.GetFrame();

    VkDevice device = VkContext::Get()->GetVkDevice();

    {
        ProfileStall("Waiting for render to finish");

        VK_VALIDATE(vkWaitForFences(device, 1, &frame.renderCompleteFence, true, UINT64_MAX));
    }
    VK_VALIDATE(vkResetFences(device, 1, &frame.renderCompleteFence));

    ResolveTimestamps();
    ResolvePipelineStatistics();

    frame.Reset();

#if defined(GPU_PROFILE_ENABLE)
    CommandBuffer::SetGPUQueryPoolTimestamp(&frame.timestampQueryPool);
#endif
}

void RenderDriver::EndFrame(Swapchain* swapchain)
{
    ProfileFunction();

    Assert(swapchain);

    for (CommandBufferPtr& cmdBuffer : m_frameData.GetFrame().usedCmdBuffers)
    {
        cmdBuffer->TryEndRendering();
    }

    SetResourceBarriers(swapchain->GetTexture());

    Frame& frame = m_frameData.GetFrame();

    std::vector<VkCommandBuffer> vkCommandBuffers;
    vkCommandBuffers.reserve(frame.usedCmdBuffers.size());
    for (const CommandBufferPtr& cmdBuffer : frame.usedCmdBuffers)
    {
        vkCommandBuffers.push_back(cmdBuffer->GetCommandBuffer().GetVkCommandBuffer());
    }

    VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = (uint32_t)vkCommandBuffers.size();
    submit.pCommandBuffers = vkCommandBuffers.data();
    if (swapchain->IsRenderable())
    {
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &frame.imageAvailabeSemaphore;
        submit.pWaitDstStageMask = &waitMask;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &frame.renderFinishedSemaphore;
    }

    {
        ProfileScope("Submitting queue");

        VK_VALIDATE(vkQueueSubmit(VkContext::Get()->GetDevice().GetGraphicsQueue(), 1, &submit, frame.renderCompleteFence));
    }

    if (swapchain->IsRenderable())
    {
        swapchain->GetVkSwapchain().Present(frame.renderFinishedSemaphore);
    }
}

bool RenderDriver::AcquireNextImage(Swapchain* swapchain)
{
    Assert(swapchain);

    Frame& frame = m_frameData.GetFrame();

    if (swapchain->GetVkSwapchain().AcquireNextImage(frame.imageAvailabeSemaphore, VK_NULL_HANDLE))
    {
        swapchain->GatherTextures();

        return true;
    }

    return false;
}

void RenderDriver::OnSwapchainBecameUnrenderable()
{
    m_frameData.DestroySwapchainSyncPrimitives();
}

void RenderDriver::OnSwapchainBecameRenderable()
{
    m_frameData.CreateSwapchainSyncPrimitives();
}

void RenderDriver::SubmitLoadCommandBuffer(CommandBufferPtr& cmdBuffer)
{
    Assert(cmdBuffer->GetCommandBuffer().GetState() == CommandBufferState::Recording);
    Assert(!cmdBuffer->m_renderPassState.isRenderingBegan);

    std::vector<CommandBufferPtr>& usedCmdBuffers = m_frameData.GetFrame().usedCmdBuffers;
    usedCmdBuffers.insert(usedCmdBuffers.begin(), std::move(cmdBuffer));
}

void RenderDriver::SubmitCommandBuffer(CommandBufferPtr& cmdBuffer)
{
    Assert(cmdBuffer->GetCommandBuffer().GetState() == CommandBufferState::Recording);

    cmdBuffer->TryEndRendering();

    m_frameData.GetFrame().usedCmdBuffers.push_back(std::move(cmdBuffer));
}

const std::vector<GPUZone>& RenderDriver::GetGPUZones() const
{
#if defined(GPU_PROFILE_ENABLE)
    return m_zones;
#else
    static std::vector<GPUZone> dummy;
    return dummy;
#endif
}

const std::vector<PipelineStatistics>& RenderDriver::GetPipelineStatistics() const
{
#if defined(GPU_PROFILE_ENABLE)
    return m_pipelineStatistics;
#else
    static std::vector<PipelineStatistics> dummy;
    return dummy;
#endif
}

void RenderDriver::WaitIdle()
{
    VkContext::Get()->GetDevice().WaitIdle();
}

RenderDriverPtr RenderDriver::Create()
{
    return std::make_unique<RenderDriver>();
}

void RenderDriver::SetResourceBarriers(TexturePtr swapchainTexture)
{
    ProfileFunction();

    Frame& frame = m_frameData.GetFrame();
    
    if (frame.usedCmdBuffers.size() <= 1)
    {
        return;
    }

    if (!frame.usedCmdBuffers[0]->m_resStates.Empty())
    {
        frame.usedCmdBuffers.insert(frame.usedCmdBuffers.begin(), CommandBuffer::Create("BARRIERS CMD BUFFER"));
    }

    for (int i = 1; i < frame.usedCmdBuffers.size(); i++)
    {
        CommandBufferPtr& cmdBuffer = frame.usedCmdBuffers[i - 1];

        ResolveBufferStates(frame.usedCmdBuffers[i]->m_resStates.buffersUsages, cmdBuffer);
        ResolveTextureStates(frame.usedCmdBuffers[i]->m_resStates.texturesUsages, cmdBuffer);

        cmdBuffer->End();
    }

    if (swapchainTexture)
    {
        TransitionSwapchainTextureToPresent(frame.usedCmdBuffers.back(), swapchainTexture);
    }

    frame.usedCmdBuffers.back()->End();
}

void RenderDriver::ResolveBufferStates(std::unordered_map<BufferPtr, BufferStates>& bufferStatesMap, CommandBufferPtr& cmdBuffer)
{
    for (auto& [buffer, bufferStates] : bufferStatesMap)
    {
        BufferState currentBufferState = buffer->GetState();

        BufferState& initial = bufferStates.initial;
        BufferState& last = bufferStates.last;

        if (currentBufferState.IsUndefined() || BufferUsageHasWrites(currentBufferState.usage) || BufferUsageHasWrites(initial.usage))
        {
            cmdBuffer->AddBufferBarrier(buffer, currentBufferState, initial);
        }

        buffer->SetState(last);
    }
}

void RenderDriver::ResolveTextureStates(std::unordered_map<TexturePtr, TextureStates>& textureStatesMap, CommandBufferPtr& cmdBuffer)
{
    for (auto& [texture, textureStates] : textureStatesMap)
    {
        TextureState currentTextureState = texture->GetState();

        TextureState& initial = textureStates.initial;
        TextureState& last = textureStates.last;

        if (currentTextureState.IsUndefined() || TextureUsageHasWrites(currentTextureState.usage) || TextureUsageHasWrites(initial.usage))
        {
            cmdBuffer->AddTextureBarrier(texture, currentTextureState, initial);
        }

        texture->SetState(last);
    }
}

void RenderDriver::TransitionSwapchainTextureToPresent(CommandBufferPtr& cmdBuffer, TexturePtr& swapchainTexture)
{
    uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    if (VkContext::Get()->GetDevice().GetGraphicsQueueIndex() != VkContext::Get()->GetDevice().GetPresentQueueIndex())
    {
        srcQueueFamilyIndex = VkContext::Get()->GetDevice().GetGraphicsQueueIndex();
        dstQueueFamilyIndex = VkContext::Get()->GetDevice().GetPresentQueueIndex();
    }

    TextureState prevState = swapchainTexture->GetState();
    TextureState newState(TextureUsagePresent);

    swapchainTexture->SetState(TextureState());

    VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    imageBarrier.srcStageMask = TextureUsageToPipelineStages(prevState.usage);
    imageBarrier.dstStageMask = TextureUsageToPipelineStages(newState.usage);
    imageBarrier.srcAccessMask = TextureUsageToAccess(prevState.usage);
    imageBarrier.dstAccessMask = TextureUsageToAccess(newState.usage);
    imageBarrier.oldLayout = TextureUsageToLayout(prevState.usage);
    imageBarrier.newLayout = TextureUsageToLayout(newState.usage);
    imageBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
    imageBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
    imageBarrier.image = swapchainTexture->GetTexture().GetVkImage();
    imageBarrier.subresourceRange.aspectMask = swapchainTexture->GetTexture().GetAspect();
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmdBuffer->GetCommandBuffer().GetVkCommandBuffer(), &dependencyInfo);
}

void RenderDriver::ResolveTimestamps()
{
#if defined(GPU_PROFILE_ENABLE)
    m_zones.clear();

    Frame& frame = m_frameData.GetFrame();

    std::vector<GPUTimestamp> timestamps;

    for (CommandBufferPtr& cmdBuffer : frame.usedCmdBuffers)
    {
        const std::vector<GPUTimestamp>& cmdBufferTimestamps = cmdBuffer->m_stats.timestamps;
        timestamps.insert(timestamps.end(), cmdBufferTimestamps.begin(), cmdBufferTimestamps.end());
    }

    std::vector<uint64_t> results = frame.timestampQueryPool.GetResults();

    struct ZoneIdxBegin
    {
        int idx = 0;
        int beginQuery = 0;
    };

    std::vector<ZoneIdxBegin> zoneIdxStack;
    
    double period = VkContext::Get()->GetPhysicalDevice().GetProperties().limits.timestampPeriod;

    int idx = 0;
    int depth = 0;
    for (const GPUTimestamp& timestamp : timestamps)
    {
        if (timestamp.name != "")
        {
            zoneIdxStack.emplace_back(idx, timestamp.query);

            GPUZone zone{ .name = timestamp.name, .depth = depth };

            m_zones.push_back(std::move(zone));

            idx++;
            depth++;
        }
        else
        {
            int zoneIdx = zoneIdxStack.back().idx;
            int beginQuery = zoneIdxStack.back().beginQuery;
            int endQuery = timestamp.query;

            zoneIdxStack.pop_back();

            uint64_t beginTimestamp = results[beginQuery];
            uint64_t endTimestamp = results[endQuery];

            GPUZone& zone = m_zones[zoneIdx];

            double msElapsed = (double)(endTimestamp - beginTimestamp) * period / 1000000.0;

            zone.durationMilliseconds = (float)msElapsed;

            depth--;
        }
    }
#endif
}

void RenderDriver::ResolvePipelineStatistics()
{
#if defined(GPU_PROFILE_ENABLE)
    m_pipelineStatistics.clear();

    Frame& frame = m_frameData.GetFrame();

    if (frame.usedCmdBuffers.empty())
    {
        return;
    }

    m_pipelineStatistics = frame.usedCmdBuffers[0]->m_stats.pipeline.GetResults();
    for (int i = 0; i < frame.usedCmdBuffers.size(); i++)
    {
        CommandBufferPtr& cmdBuffer = frame.usedCmdBuffers[i];

        std::vector<PipelineStatistics> stats = cmdBuffer->m_stats.pipeline.GetResults();

        if (stats.empty())
        {
            continue;
        }

        for (int j = 0; j < m_pipelineStatistics.size(); j++)
        {
            m_pipelineStatistics[j].count += stats[j].count;
        }
    }
#endif
}