#include "PipelineGraphicsState.h"

#include "RenderConstants.h"

#include <Framework/Hash.h>

#define CALCULATE_HASH(x) hash = JenkinsHashContinue(hash, &x, sizeof(x));

PipelineRenderingState::PipelineRenderingState()
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    state.pColorAttachmentFormats = formats.data();

    Reset();
}

PipelineRenderingState::PipelineRenderingState(const PipelineRenderingState& other)
{
    CopyFrom(other);
}

PipelineRenderingState& PipelineRenderingState::operator=(const PipelineRenderingState& other)
{
    CopyFrom(other);
    return *this;
}

bool PipelineRenderingState::operator==(const PipelineRenderingState& other) const
{
    constexpr int offset = offsetof(VkPipelineRenderingCreateInfo, viewMask);
    constexpr int size = offsetof(VkPipelineRenderingCreateInfo, pColorAttachmentFormats) - offset;

    if (0 != memcmp((char*)&state + offset, (char*)&other.state + offset, size))
    {
        return false;
    }

    constexpr int offset2 = offsetof(VkPipelineRenderingCreateInfo, depthAttachmentFormat);
    constexpr int size2 = offsetof(VkPipelineRenderingCreateInfo, stencilAttachmentFormat) - offset2;

    if (0 != memcmp((char*)&state + offset2, (char*)&other.state + offset2, size2))
    {
        return false;
    }

    for (int i = 0; i < (int)state.colorAttachmentCount; i++)
    {
        if (formats[i] != other.formats[i])
        {
            return false;
        }
    }

    return true;
}

bool PipelineRenderingState::operator!=(const PipelineRenderingState& other) const
{
    return !(*this == other);
}

void PipelineRenderingState::Reset()
{
    state.pNext = nullptr;
    state.viewMask = 0;
    state.colorAttachmentCount = 0;
    for (int i = 0; i < MAX_RENDER_TARGETS_COUNT; i++)
    {
        formats[i] = VK_FORMAT_UNDEFINED;
    }
    state.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    state.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
}

const VkPipelineRenderingCreateInfo* PipelineRenderingState::GetPtr()
{
    return &state;
}

uint64_t PipelineRenderingState::CalculateHash(uint64_t hash) const
{
    CALCULATE_HASH(state.viewMask);
    CALCULATE_HASH(state.colorAttachmentCount);

    for (size_t i = 0; i < state.colorAttachmentCount; i++)
    {
        CALCULATE_HASH(formats[i]);
    }

    CALCULATE_HASH(state.depthAttachmentFormat);
    CALCULATE_HASH(state.stencilAttachmentFormat);

    return hash;
}

void PipelineRenderingState::CopyFrom(const PipelineRenderingState& other)
{
    memcpy(formats.data(), other.formats.data(), sizeof(formats));
    memcpy(&state, &other.state, sizeof(state));
    state.pColorAttachmentFormats = formats.data();
}

PipelineInputAssemblyState::PipelineInputAssemblyState()
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    Reset();
}

bool PipelineInputAssemblyState::operator==(const PipelineInputAssemblyState& other) const
{
    constexpr int offset = offsetof(VkPipelineInputAssemblyStateCreateInfo, topology);
    constexpr int size = sizeof(VkPipelineInputAssemblyStateCreateInfo) - offset;

    return 0 == memcmp((char*)&state + offset, (char*)&other.state + offset, size);
}

bool PipelineInputAssemblyState::operator!=(const PipelineInputAssemblyState& other) const
{
    return !(*this == other);
}

void PipelineInputAssemblyState::Reset()
{
    state.pNext = nullptr;
    state.flags = 0;
    state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    state.primitiveRestartEnable = VK_FALSE;
}

const VkPipelineInputAssemblyStateCreateInfo* PipelineInputAssemblyState::GetPtr()
{
    return &state;
}

uint64_t PipelineInputAssemblyState::CalculateHash(uint64_t hash) const
{
    CALCULATE_HASH(state.topology);
    CALCULATE_HASH(state.primitiveRestartEnable);

    return hash;
}

PipelineRasterizationState::PipelineRasterizationState()
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    Reset();
}

bool PipelineRasterizationState::operator==(const PipelineRasterizationState& other) const
{
    constexpr int offset = offsetof(VkPipelineRasterizationStateCreateInfo, depthClampEnable);
    constexpr int size = sizeof(VkPipelineRasterizationStateCreateInfo) - offset;

    return 0 == memcmp((char*)&state + offset, (char*)&other.state + offset, size);
}

bool PipelineRasterizationState::operator!=(const PipelineRasterizationState& other) const
{
    return !(*this == other);
}

void PipelineRasterizationState::SetCull(CullMode mode, FrontFace frontFace)
{
    state.cullMode = (VkCullModeFlagBits)mode;
    state.frontFace = (VkFrontFace)frontFace;
}

void PipelineRasterizationState::Reset()
{
    state.pNext = nullptr;
    state.flags = 0;
    state.depthClampEnable = VK_FALSE;
    state.rasterizerDiscardEnable = VK_FALSE;
    state.polygonMode = VK_POLYGON_MODE_FILL;
    state.cullMode = VK_CULL_MODE_NONE;
    state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    state.depthBiasEnable = VK_FALSE;
    state.depthBiasConstantFactor = 0.0f;
    state.depthBiasClamp = 0.0f;
    state.depthBiasSlopeFactor = 0.0f;
    state.lineWidth = 1.0f;
}

const VkPipelineRasterizationStateCreateInfo* PipelineRasterizationState::GetPtr() const
{
    return &state;
}

uint64_t PipelineRasterizationState::CalculateHash(uint64_t hash) const
{
    constexpr size_t offset = offsetof(VkPipelineRasterizationStateCreateInfo, depthClampEnable);
    constexpr size_t size = sizeof(VkPipelineRasterizationStateCreateInfo) - offset;

    hash = JenkinsHashContinue(hash, &state.depthClampEnable, size);

    return hash;
}

PipelineDepthStencilState::PipelineDepthStencilState()
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    Reset();
}

bool PipelineDepthStencilState::operator==(const PipelineDepthStencilState& other) const
{
    constexpr int offset = offsetof(VkPipelineDepthStencilStateCreateInfo, depthTestEnable);
    constexpr int size = sizeof(VkPipelineRasterizationStateCreateInfo) - offset;

    return 0 == memcmp((char*)&state + offset, (char*)&other.state + offset, size);
}

bool PipelineDepthStencilState::operator!=(const PipelineDepthStencilState& other) const
{
    return !(*this == other);
}

void PipelineDepthStencilState::SetDepthTest(bool depthWriteEnable, CompareOp depthCompareOp)
{
    state.depthTestEnable = true;
    state.depthWriteEnable = depthWriteEnable;
    state.depthCompareOp = (VkCompareOp)depthCompareOp;
}

void PipelineDepthStencilState::SetDepthBoundsTest(float minDepth, float maxDepth)
{
    state.depthBoundsTestEnable = VK_TRUE;
    state.minDepthBounds = minDepth;
    state.maxDepthBounds = maxDepth;
}

void PipelineDepthStencilState::Reset()
{
    state.pNext = nullptr;
    state.flags = 0;
    state.depthTestEnable = VK_FALSE;
    state.depthWriteEnable = VK_FALSE;
    state.depthCompareOp = VK_COMPARE_OP_NEVER;
    state.depthBoundsTestEnable = VK_FALSE;
    state.stencilTestEnable = VK_FALSE;
    state.front.failOp = VK_STENCIL_OP_KEEP;
    state.front.passOp = VK_STENCIL_OP_KEEP;
    state.front.depthFailOp = VK_STENCIL_OP_KEEP;
    state.front.compareOp = VK_COMPARE_OP_NEVER;
    state.front.compareMask = 0;
    state.front.writeMask = 0;
    state.front.reference = 0;
    state.back.failOp = VK_STENCIL_OP_KEEP;
    state.back.passOp = VK_STENCIL_OP_KEEP;
    state.back.depthFailOp = VK_STENCIL_OP_KEEP;
    state.back.compareOp = VK_COMPARE_OP_NEVER;
    state.back.compareMask = 0;
    state.back.writeMask = 0;
    state.back.reference = 0;
    state.minDepthBounds = 0.0f;
    state.maxDepthBounds = 0.0f;
}

const VkPipelineDepthStencilStateCreateInfo* PipelineDepthStencilState::GetPtr()
{
    return &state;
}

uint64_t PipelineDepthStencilState::CalculateHash(uint64_t hash) const
{
    constexpr size_t offset = offsetof(VkPipelineDepthStencilStateCreateInfo, depthTestEnable);
    constexpr size_t size = sizeof(VkPipelineDepthStencilStateCreateInfo) - offset;

    hash = JenkinsHashContinue(hash, &state.depthTestEnable, size);

    return hash;
}

PipelineBlendState::PipelineBlendState()
{
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    state.pAttachments = blendAttachments.data();

    Reset();
}

PipelineBlendState::PipelineBlendState(const PipelineBlendState& other)
{
    CopyFrom(other);
}

PipelineBlendState& PipelineBlendState::operator=(const PipelineBlendState& other)
{
    CopyFrom(other);
    return *this;
}

bool PipelineBlendState::operator==(const PipelineBlendState& other) const
{
    if (state.attachmentCount != other.state.attachmentCount)
    {
        return false;
    }

    if (state.logicOpEnable == VK_TRUE && other.state.logicOpEnable == VK_TRUE)
    {
        if (state.logicOp != other.state.logicOp)
        {
            return false;
        }
    }
    else
    {
        if (state.logicOpEnable != other.state.logicOpEnable)
        {
            return false;
        }
    }

    for (int i = 0; i < (int)state.attachmentCount; i++)
    {
        if (blendAttachments[i].blendEnable == VK_TRUE && other.blendAttachments[i].blendEnable == VK_TRUE)
        {
            constexpr int offset = offsetof(VkPipelineColorBlendAttachmentState, srcColorBlendFactor);
            constexpr int size = sizeof(VkPipelineColorBlendAttachmentState) - offset;

            if (0 != memcmp((char*)&blendAttachments[i] + offset, (char*)&other.blendAttachments[i] + offset, size))
            {
                return false;
            }
        }
        else
        {
            if (blendAttachments[i].blendEnable != other.blendAttachments[i].blendEnable)
            {
                return false;
            }
        }
    }

    return true;
}

bool PipelineBlendState::operator!=(const PipelineBlendState& other) const
{
    return !(*this == other);
}

void PipelineBlendState::SetBlendingColor(int slot, BlendFactor srcColor, BlendFactor dstColor, BlendOp op)
{
    Assert(slot >= 0 && slot < MAX_RENDER_TARGETS_COUNT);

    VkPipelineColorBlendAttachmentState& blendAttachmentState = blendAttachments[slot];
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.srcColorBlendFactor = (VkBlendFactor)srcColor;
    blendAttachmentState.dstColorBlendFactor = (VkBlendFactor)dstColor;
    blendAttachmentState.colorBlendOp = (VkBlendOp)op;
}

void PipelineBlendState::SetBlendingAlpha(int slot, BlendFactor srcAlpha, BlendFactor dstAlpha, BlendOp op)
{
    Assert(slot >= 0 && slot < MAX_RENDER_TARGETS_COUNT);

    VkPipelineColorBlendAttachmentState& blendAttachmentState = blendAttachments[slot];
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.srcAlphaBlendFactor = (VkBlendFactor)srcAlpha;
    blendAttachmentState.dstAlphaBlendFactor = (VkBlendFactor)dstAlpha;
    blendAttachmentState.alphaBlendOp = (VkBlendOp)op;
}

void PipelineBlendState::Reset()
{
    state.pNext = nullptr;
    state.flags = 0;
    state.logicOpEnable = VK_FALSE;
    state.logicOp = VK_LOGIC_OP_CLEAR;
    state.attachmentCount = 0;
    state.blendConstants[0] = 0.0f;
    state.blendConstants[1] = 0.0f;
    state.blendConstants[2] = 0.0f;
    state.blendConstants[3] = 0.0f;

    for (int i = 0; i < MAX_RENDER_TARGETS_COUNT; i++)
    {
        VkPipelineColorBlendAttachmentState& attachment = blendAttachments[i];

        attachment.blendEnable = VK_FALSE;
        attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachment.colorBlendOp = VK_BLEND_OP_ADD;
        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }
}

const VkPipelineColorBlendStateCreateInfo* PipelineBlendState::GetPtr()
{
    return &state;
}

uint64_t PipelineBlendState::CalculateHash(uint64_t hash) const
{
    constexpr size_t offset = offsetof(VkPipelineColorBlendStateCreateInfo, logicOpEnable);
    constexpr size_t size = offsetof(VkPipelineColorBlendStateCreateInfo, pAttachments) - offset;

    hash = JenkinsHashContinue(hash, &state.logicOpEnable, size);

    for (size_t i = 0; i < state.attachmentCount; i++)
    {
        CALCULATE_HASH(blendAttachments[i]);
    }
    
    return hash;
}

void PipelineBlendState::CopyFrom(const PipelineBlendState& other)
{
    memcpy(&state, &other.state, sizeof(state));
    memcpy(blendAttachments.data(), other.blendAttachments.data(), sizeof(blendAttachments));
    state.pAttachments = blendAttachments.data();
}

PipelineGraphicsState::PipelineGraphicsState()
{
    Reset();
}

bool PipelineGraphicsState::operator==(const PipelineGraphicsState& other) const
{
    if (m_shader != other.m_shader)
    {
        return false;
    }

    if (m_renderingState != other.m_renderingState)
    {
        return false;
    }
    if (m_inputAssemblyState != other.m_inputAssemblyState)
    {
        return false;
    }
    if (m_rasterizationState != other.m_rasterizationState)
    {
        return false;
    }
    if (m_blendState != other.m_blendState)
    {
        return false;
    }

    return true;
}

bool PipelineGraphicsState::operator!=(const PipelineGraphicsState& other) const
{
    return !(*this == other);
}

Shader* PipelineGraphicsState::GetShader() const
{
    return m_shader;
}

void PipelineGraphicsState::SetShader(Shader* shader)
{
    m_shader = shader;

    m_isShaderDirty = true;
}

bool PipelineGraphicsState::IsShaderDirty() const
{
    return m_isShaderDirty;
}

void PipelineGraphicsState::SetShaderDirty(bool isDirty)
{
    m_isShaderDirty = isDirty;
}

PipelineRenderingState& PipelineGraphicsState::GetRenderingState()
{
    return m_renderingState;
}

PipelineInputAssemblyState& PipelineGraphicsState::GetInputAssemblyState()
{
    return m_inputAssemblyState;
}

PipelineRasterizationState& PipelineGraphicsState::GetRasterizationState()
{
    return m_rasterizationState;
}

PipelineDepthStencilState& PipelineGraphicsState::GetDepthStencilState()
{
    return m_depthStencilState;
}

PipelineBlendState& PipelineGraphicsState::GetBlendState()
{
    return m_blendState;
}

void PipelineGraphicsState::Reset()
{
    m_shader = nullptr;

    m_renderingState.Reset();
    m_inputAssemblyState.Reset();
    m_rasterizationState.Reset();
    m_depthStencilState.Reset();
    m_blendState.Reset();

    m_isShaderDirty = true;
}

uint32_t PipelineGraphicsState::CalculateHash() const
{
    ProfileFunction();

    uint64_t hash = 0;
    CALCULATE_HASH(m_shader);
    
    hash = m_renderingState.CalculateHash(hash);
    hash = m_inputAssemblyState.CalculateHash(hash);
    hash = m_rasterizationState.CalculateHash(hash);
    hash = m_depthStencilState.CalculateHash(hash);
    hash = m_blendState.CalculateHash(hash);

    hash = JenkinsHashEnd(hash);

    return (uint32_t)hash;
}

void PipelineGraphicsDynamicState::Reset()
{
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 0.0f;
    viewport.height = 0.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 0.0f;

    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = 0;
    scissor.extent.height = 0;
}

#undef CALCULATE_HASH