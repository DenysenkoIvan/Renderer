#pragma once

#include "Shader.h"

class PSOCompute
{
public:
    NON_COPYABLE_MOVABLE(PSOCompute);

    PSOCompute(const Shader* shader);
    ~PSOCompute();

    VkPipeline GetPipeline() const;
    VkPipelineLayout GetPipelineLayout() const;
    const Shader* GetShader() const;

    static const PSOCompute* Get(const Shader* shader);
    static void RecreateOnShaderChanged(const std::unordered_set<const Shader*>& changedShaders);

    static void DestroyCache();

private:
    void CreatePipeline();

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    const Shader* m_shader = nullptr;

    static std::unordered_map<void*, std::unique_ptr<PSOCompute>> s_psoCache;
};