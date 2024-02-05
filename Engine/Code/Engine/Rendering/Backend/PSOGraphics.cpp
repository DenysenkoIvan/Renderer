#include "PSOGraphics.h"

#include "VulkanImpl/VkContext.h"
#include "VulkanImpl/VulkanValidation.h"

#include "DescriptorBindingTable.h"

std::unordered_map<uint32_t, std::unique_ptr<PSOGraphics>> PSOGraphics::s_psoCache;

PSOGraphics::PSOGraphics(const PipelineGraphicsState& state)
    : m_state(state)
{
    const auto& renderingState = m_state.GetRenderingState().state;
    Assert(renderingState.colorAttachmentCount == m_state.GetBlendState().state.attachmentCount || renderingState.depthAttachmentFormat != VK_FORMAT_UNDEFINED, "Blend attachment count is zero");

    CreatePipeline();
}

PSOGraphics::~PSOGraphics()
{
    if (m_pipeline)
    {
        vkDestroyPipeline(VkContext::Get()->GetVkDevice(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
}

VkPipeline PSOGraphics::GetPipeline() const
{
    return m_pipeline;
}

const Shader* PSOGraphics::GetShader() const
{
    return m_state.GetShader();
}

const PSOGraphics* PSOGraphics::Get(const PipelineGraphicsState& state)
{
    ProfileFunction();

    uint32_t hash = state.CalculateHash();

    auto it = s_psoCache.find(hash);
    if (it != s_psoCache.end())
    {
        return it->second.get();
    }

    if (!state.GetShader())
    {
        LogError("PSOGraphics creation failed. Shader must be bount")
        return nullptr;
    }

    std::unique_ptr<PSOGraphics> pso = std::make_unique<PSOGraphics>(state);
    
    if (pso->m_pipeline)
    {
        const PSOGraphics* retValue = pso.get();
        s_psoCache.emplace(hash, std::move(pso));
        return retValue;
    }
    
    return nullptr;
}

void PSOGraphics::DestroyCache()
{
    s_psoCache.clear();
}

void PSOGraphics::RecreateOnShaderChanged(const std::unordered_set<const Shader*>& changedShaders)
{
    for (auto& [hash, pso] : s_psoCache)
    {
        if (!changedShaders.contains(pso->GetShader()))
        {
            continue;
        }

        if (pso->m_pipeline)
        {
            vkDestroyPipeline(VkContext::Get()->GetVkDevice(), pso->m_pipeline, nullptr);
            pso->m_pipeline = VK_NULL_HANDLE;
        }

        pso->CreatePipeline();
    }
}

void PSOGraphics::CreatePipeline()
{
    Assert(m_state.GetShader());

    std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages;
    int shaderStagesCount = 0;

    if (m_state.GetShader()->GetTaskStage())
    {
        shaderStages[shaderStagesCount++] = m_state.GetShader()->GetTaskStage()->GetVkShader()->GetPipelineStageCreateInfo();
    }
    if (m_state.GetShader()->GetMeshStage())
    {
        shaderStages[shaderStagesCount++] = m_state.GetShader()->GetMeshStage()->GetVkShader()->GetPipelineStageCreateInfo();
    }
    else if (m_state.GetShader()->GetVertexStage())
    {
        shaderStages[shaderStagesCount++] = m_state.GetShader()->GetVertexStage()->GetVkShader()->GetPipelineStageCreateInfo();
    }
    if (m_state.GetShader()->GetPixelStage())
    {
        shaderStages[shaderStagesCount++] = m_state.GetShader()->GetPixelStage()->GetVkShader()->GetPipelineStageCreateInfo();
    }

    VkPipelineVertexInputStateCreateInfo vertexInputState = CreateVertexInputState();
    VkPipelineViewportStateCreateInfo viewportState = CreateViewportState();
    VkPipelineMultisampleStateCreateInfo multisampleState = CreateMultisampleState();
    VkPipelineDynamicStateCreateInfo dynamicStatesState = CreateDynamicStatesState();

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{ .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineCreateInfo.pNext = m_state.GetRenderingState().GetPtr();
    pipelineCreateInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    pipelineCreateInfo.stageCount = shaderStagesCount;
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = m_state.GetInputAssemblyState().GetPtr();
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = m_state.GetRasterizationState().GetPtr();
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pDepthStencilState = m_state.GetDepthStencilState().GetPtr();
    pipelineCreateInfo.pColorBlendState = m_state.GetBlendState().GetPtr();
    pipelineCreateInfo.pDynamicState = &dynamicStatesState;
    pipelineCreateInfo.layout = DescriptorBindingTable::Get()->GetPipelineLayout();

    VK_VALIDATE(vkCreateGraphicsPipelines(VkContext::Get()->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline));
}

VkPipelineVertexInputStateCreateInfo PSOGraphics::CreateVertexInputState()
{
    VkPipelineVertexInputStateCreateInfo state{};
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    state.pNext = nullptr;
    state.flags = 0;
    state.vertexBindingDescriptionCount = 0;
    state.pVertexBindingDescriptions = nullptr;
    state.vertexAttributeDescriptionCount = 0;
    state.pVertexAttributeDescriptions = nullptr;

    return state;
}

VkPipelineViewportStateCreateInfo PSOGraphics::CreateViewportState()
{
    VkPipelineViewportStateCreateInfo state{};
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    state.pNext = nullptr;
    state.flags = 0;
    state.viewportCount = 0;
    state.pViewports = nullptr;
    state.scissorCount = 0;
    state.pScissors = nullptr;

    return state;
}

VkPipelineMultisampleStateCreateInfo PSOGraphics::CreateMultisampleState()
{
    VkPipelineMultisampleStateCreateInfo state{};
    state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    state.pNext = nullptr;
    state.flags = 0;
    state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    state.sampleShadingEnable = VK_FALSE;

    return state;
}

VkPipelineDynamicStateCreateInfo PSOGraphics::CreateDynamicStatesState()
{
    static std::array<VkDynamicState, 2> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT
    };

    VkPipelineDynamicStateCreateInfo dynamicStatesState{};
    dynamicStatesState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStatesState.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicStatesState.pDynamicStates = dynamicStates.data();

    return dynamicStatesState;
}