#pragma once

#include "PipelineGraphicsState.h"

class PSOGraphics
{
public:
    NON_COPYABLE_MOVABLE(PSOGraphics);

    PSOGraphics(const PipelineGraphicsState& state);
    ~PSOGraphics();

    VkPipeline GetPipeline() const;
    const Shader* GetShader() const;

    static const PSOGraphics* Get(const PipelineGraphicsState& state);

    static void DestroyCache();
    static void RecreateOnShaderChanged(const std::unordered_set<const Shader*>& changedShaders);

private:
    void CreatePipeline();

    VkPipelineVertexInputStateCreateInfo CreateVertexInputState();
    VkPipelineViewportStateCreateInfo CreateViewportState();
    VkPipelineMultisampleStateCreateInfo CreateMultisampleState();
    VkPipelineDynamicStateCreateInfo CreateDynamicStatesState();

private:
    PipelineGraphicsState m_state{};
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    static std::unordered_map<uint32_t, std::unique_ptr<PSOGraphics>> s_psoCache;
};