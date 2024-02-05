#pragma once

#include <array>

#include "Common.h"
#include "RenderConstants.h"
#include "Shader.h"

struct PipelineRenderingState
{
    VkPipelineRenderingCreateInfo state{};
    std::array<VkFormat, MAX_RENDER_TARGETS_COUNT> formats;

    PipelineRenderingState();
    PipelineRenderingState(const PipelineRenderingState& other);
    PipelineRenderingState& operator=(const PipelineRenderingState& other);

    bool operator==(const PipelineRenderingState& other) const;
    bool operator!=(const PipelineRenderingState& other) const;

    void Reset();

    const VkPipelineRenderingCreateInfo* GetPtr();

    inline uint64_t CalculateHash(uint64_t hash) const;

private:
    void CopyFrom(const PipelineRenderingState& other);
};

struct PipelineInputAssemblyState
{
    VkPipelineInputAssemblyStateCreateInfo state{};

    PipelineInputAssemblyState();

    bool operator==(const PipelineInputAssemblyState& other) const;
    bool operator!=(const PipelineInputAssemblyState& other) const;

    void Reset();

    const VkPipelineInputAssemblyStateCreateInfo* GetPtr();

    inline uint64_t CalculateHash(uint64_t hash) const;
};

struct PipelineRasterizationState
{
    VkPipelineRasterizationStateCreateInfo state{};

    PipelineRasterizationState();

    bool operator==(const PipelineRasterizationState& other) const;
    bool operator!=(const PipelineRasterizationState& other) const;

    void SetCull(CullMode mode, FrontFace frontFace);

    void Reset();

    const VkPipelineRasterizationStateCreateInfo* GetPtr() const;

    inline uint64_t CalculateHash(uint64_t hash) const;
};

struct PipelineDepthStencilState
{
    VkPipelineDepthStencilStateCreateInfo state{};

    PipelineDepthStencilState();

    bool operator==(const PipelineDepthStencilState& other) const;
    bool operator!=(const PipelineDepthStencilState& other) const;

    void SetDepthTest(bool depthWriteEnable, CompareOp depthCompareOp);
    void SetDepthBoundsTest(float minDepth, float maxDepth);

    void Reset();

    const VkPipelineDepthStencilStateCreateInfo* GetPtr();

    inline uint64_t CalculateHash(uint64_t hash) const;
};

struct PipelineBlendState
{
    VkPipelineColorBlendStateCreateInfo state{};
    std::array<VkPipelineColorBlendAttachmentState, MAX_RENDER_TARGETS_COUNT> blendAttachments;

    PipelineBlendState();
    PipelineBlendState(const PipelineBlendState& other);
    PipelineBlendState& operator=(const PipelineBlendState& other);

    bool operator==(const PipelineBlendState& other) const;
    bool operator!=(const PipelineBlendState& other) const;

    void SetBlendingColor(int slot, BlendFactor srcColor, BlendFactor dstColor, BlendOp op);
    void SetBlendingAlpha(int slot, BlendFactor srcAlpha, BlendFactor dstAlpha, BlendOp op);

    void Reset();

    const VkPipelineColorBlendStateCreateInfo* GetPtr();

    inline uint64_t CalculateHash(uint64_t hash) const;

private:
    void CopyFrom(const PipelineBlendState& other);
};

class PipelineGraphicsState
{
public:
    PipelineGraphicsState();

    bool operator==(const PipelineGraphicsState& other) const;
    bool operator!=(const PipelineGraphicsState& other) const;

    Shader* GetShader() const;
    void SetShader(Shader* shader);

    bool IsShaderDirty() const;
    void SetShaderDirty(bool isDirty);

    PipelineRenderingState& GetRenderingState();
    PipelineInputAssemblyState& GetInputAssemblyState();
    PipelineRasterizationState& GetRasterizationState();
    PipelineDepthStencilState& GetDepthStencilState();
    PipelineBlendState& GetBlendState();

    void Reset();

    uint32_t CalculateHash() const;

private:
    Shader* m_shader = nullptr;

    PipelineRenderingState m_renderingState{};
    PipelineInputAssemblyState m_inputAssemblyState{};
    PipelineRasterizationState m_rasterizationState{};
    PipelineDepthStencilState m_depthStencilState{};
    PipelineBlendState m_blendState{};

    bool m_isShaderDirty = true;
};

struct PipelineGraphicsDynamicState
{
    VkViewport viewport{};
    VkRect2D scissor{};

    void Reset();
};