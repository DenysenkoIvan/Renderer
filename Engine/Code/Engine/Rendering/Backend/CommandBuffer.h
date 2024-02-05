#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "VulkanImpl/VulkanCommandBuffer.h"

#include "Buffer.h"
#include "Common.h"
#include "CommonResources.h"
#include "GPUQueryPipelineStatistics.h"
#include "GPUQueryPoolTimestamp.h"
#include "PipelineGraphicsState.h"
#include "PipelineComputeState.h"
#include "RenderConstants.h"
#include "Sampler.h"
#include "Shader.h"
#include "Texture.h"
#include "PSOGraphics.h"
#include "PSOCompute.h"

class RenderDriver;

namespace
{
    struct RenderPassState
    {
        std::array<TexturePtr, MAX_RENDER_TARGETS_COUNT> renderTargets{};
        std::array<VkRenderingAttachmentInfo, MAX_RENDER_TARGETS_COUNT> attachmentInfos{};
        std::array<VkImageSubresourceRange, MAX_RENDER_TARGETS_COUNT> subresources{};
        uint8_t attachmentsUsed = 0;
        TexturePtr depthTarget = nullptr;
        VkRenderingAttachmentInfo depthAttachmentInfo{};
        bool isRenderingBegan = false;

        RenderPassState();

        void SetRenderTarget(int slot, TexturePtr renderTarget, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE, const glm::vec4& clearColor = glm::vec4(0.0f), int baseLayer = 0);
        void SetDepthTarget(TexturePtr depthTarget, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE, float depthClearValue = 1.0f, uint32_t stencilClearValue = 0);

        void Clear();
    };

    struct BufferStates
    {
        BufferState initial{};
        BufferState last{};
    };

    struct TextureStates
    {
        TextureState initial{};
        TextureState last{};
    };

    struct BoundResources
    {
        const PSOGraphics* psoGraphics = nullptr;
        const PSOCompute* psoCompute = nullptr;
        bool isPsoGraphicsDirty = true;
        bool isPsoComputeDirty = true;

        void Reset();

        void SetPsoGraphics(const PSOGraphics* pso);
        bool HasPsoGraphics();

        void SetPsoCompute(const PSOCompute* pso);
        bool HasPsoCompute();
    };
}

struct ResourcesStates
{
    std::unordered_map<BufferPtr, BufferStates> buffersUsages;
    std::unordered_map<TexturePtr, TextureStates> texturesUsages;

    void Reset();

    bool Empty();
};

struct GPUTimestamp
{
    std::string name;
    int query = 0;

    GPUTimestamp(const char* name, int query);
};

struct GPUZone
{
    std::string name;
    float durationMilliseconds = 0.0f;
    int depth = 0;

    bool operator==(const std::string& other) const;
};

struct CmdBufferStatistics
{
    GPUQueryPipelineStatistics pipeline;
    std::vector<GPUTimestamp> timestamps;

    void Create();
    void Destroy();

    void Reset();
};

class CommandBuffer;
using CommandBufferPtr = std::shared_ptr<CommandBuffer>;

class CommandBuffer
{
    friend class RenderDriver;
    friend class DescriptorBindingTable;

public:
    NON_COPYABLE_MOVABLE(CommandBuffer);

    CommandBuffer() = default;
    ~CommandBuffer();

    void Begin();
    void End();
    
    void Reset();

    void SetRenderTarget(int slot, TexturePtr renderTarget, bool isPreserveContent = true);
    void SetRenderTargetClear(int slot, TexturePtr renderTarget, const glm::vec4& clearColor);
    void SetRenderTargetLayer(int slot, int layer, TexturePtr renderTarget, bool isPreserveContent = true);
    void SetDepthTarget(TexturePtr depthTarget, bool isPreserveContent = true);
    void SetDepthTargetClear(TexturePtr depthTarget, float depthClearValue, uint32_t stencilClearValue);

    void PropagateRenderingInfo(PipelineGraphicsState& state);

    void BeginRenderPass();
    void EndRenderPass();

    void BindPsoGraphics(const PSOGraphics* pso);
    void BindPsoCompute(const PSOCompute* pso);

    void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
    void SetScissor(int offsetX, int offsetY, int width, int height);

    void PushConstants(const void* data, size_t size);

    void Draw(uint32_t vtxCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    void DrawMeshTasks(uint32_t x, uint32_t y, uint32_t z);

    void Dispatch(int x, int y, int z);

    void ResetBindAndRenderStates();

    void CopyToBuffer(BufferPtr buffer, const void* data, size_t size);
    void CopyToTexture(TexturePtr texture, const void* ptr, size_t sizeBytes);
    void GenerateMipmaps(TexturePtr texture);

    void RegisterSRVUsageBuffer(BufferPtr buffer);
    void RegisterSRVUsageTexture(TexturePtr texture);
    void RegisterUAVUsageBuffer(BufferPtr buffer);
    void RegisterUAVUsageTexture(TexturePtr texture);

    void MarkerBegin(const char* markerName = nullptr);
    void MarkerEnd();

    void BeginZone(const char* name);
    void EndZone();

    VulkanCommandBuffer& GetCommandBuffer();

    static CommandBufferPtr Create(const char* labelName);
    static void DestroyPool();
    static void SetGPUQueryPoolTimestamp(GPUQueryPoolTimestamp* pool);

private:
    void Init();
    void Destroy();

    void TryBeginRendering();
    void BeginRendering();
    void TryEndRendering();

    void SetDynamicStates();

    VkRect2D GetRenderArea() const;

    BufferPtr GetStagingBuffer(size_t sizeBytes);

    void AddBufferUsage(const BufferPtr& buffer, BufferUsageFlags newUsage, ShaderStageFlags newStages = 0);
    void AddTextureUsage(const TexturePtr& texture, TextureUsageFlags newUsage);

    void AddBufferBarrier(const BufferPtr& buffer, const BufferState& prevState, const BufferState& newState);
    void AddTextureBarrier(const TexturePtr& texture, const TextureState& prevState, const TextureState& newState);

    void ValidateIsInRecordingState();

private:
    VulkanCommandBuffer m_commandBuffer;
    std::string m_name;

    RenderPassState m_renderPassState;
    PipelineGraphicsDynamicState m_stateGraphicsDynamic;
    PipelineComputeState m_stateCompute;
    BoundResources m_boundRes;

    ResourcesStates m_resStates;

    std::vector<BufferPtr> m_stagingBuffers;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

#if defined(GPU_PROFILE_ENABLE)
    CmdBufferStatistics m_stats;
#endif

    static std::vector<CommandBufferPtr> s_commandBuffersPool;
#if defined(GPU_PROFILE_ENABLE)
    static GPUQueryPoolTimestamp* s_queryPoolTimestamp;
#endif
};