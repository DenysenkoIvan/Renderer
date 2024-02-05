#pragma once

#include <array>
#include <memory>

#include "CommandBuffer.h"
#include "GPUQueryPoolTimestamp.h"
#include "GPUQueryPipelineStatistics.h"
#include "Swapchain.h"

#include "VulkanImpl/VkContext.h"

namespace
{
    struct Frame
    {
        NON_COPYABLE_MOVABLE(Frame);

        VkSemaphore imageAvailabeSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence renderCompleteFence = VK_NULL_HANDLE;
        std::vector<CommandBufferPtr> usedCmdBuffers;
#if defined(GPU_PROFILE_ENABLE)
        GPUQueryPoolTimestamp timestampQueryPool;
#endif

        Frame() = default;

        void Create();
        void Destroy();

        void Reset();
    };

    struct FrameData
    {
        NON_COPYABLE_MOVABLE(FrameData);

        FrameData() = default;

        std::array<Frame, FRAME_COUNT> frames;
        uint32_t frameIndex = -1;

        void Create();

        void Destroy();

        void CreateSwapchainSyncPrimitives();
        void DestroySwapchainSyncPrimitives();

        uint32_t GetFrameIndex();
        Frame& GetFrame();

        void NextFrame();
    };
};

class RenderDriver;
using RenderDriverPtr = std::unique_ptr<RenderDriver>;

class RenderDriver
{
public:
    NON_COPYABLE_MOVABLE(RenderDriver);

    RenderDriver();
    ~RenderDriver();

    void DestroyFramesData();

    void BeginFrame();
    void EndFrame(Swapchain* swapchain);

    bool AcquireNextImage(Swapchain* swapchain);
    void OnSwapchainBecameUnrenderable();
    void OnSwapchainBecameRenderable();

    void SubmitLoadCommandBuffer(CommandBufferPtr& cmdBuffer);
    void SubmitCommandBuffer(CommandBufferPtr& cmdBuffer);

    const std::vector<GPUZone>& GetGPUZones() const;
    const std::vector<PipelineStatistics>& GetPipelineStatistics() const;

    void WaitIdle();

    static RenderDriverPtr Create();

private:
    void SetResourceBarriers(TexturePtr swapchainTexture);
    void ResolveBufferStates(std::unordered_map<BufferPtr, BufferStates>& bufferStatesMap, CommandBufferPtr& cmdBuffer);
    void ResolveTextureStates(std::unordered_map<TexturePtr, TextureStates>& textureStatesMap, CommandBufferPtr& cmdBuffer);
    void TransitionSwapchainTextureToPresent(CommandBufferPtr& cmdBuffer, TexturePtr& swapchainTexture);

    void ResolveTimestamps();
    void ResolvePipelineStatistics();

private:
    VkContext m_context;
    FrameData m_frameData;

    std::vector<GPUZone> m_zones;
    std::vector<PipelineStatistics> m_pipelineStatistics;
};