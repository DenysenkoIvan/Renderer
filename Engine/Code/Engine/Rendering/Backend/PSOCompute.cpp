#include "PSOCompute.h"

#include "VulkanImpl/VkContext.h"
#include "VulkanImpl/VulkanValidation.h"

#include "DescriptorBindingTable.h"

std::unordered_map<void*, std::unique_ptr<PSOCompute>> PSOCompute::s_psoCache;

PSOCompute::PSOCompute(const Shader* shader)
    : m_shader(shader)
{
    CreatePipeline();
}

PSOCompute::~PSOCompute()
{
    VkDevice device = VkContext::Get()->GetVkDevice();

    if (m_pipelineLayout)
    {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (m_pipeline)
    {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
}

VkPipeline PSOCompute::GetPipeline() const
{
    return m_pipeline;
}

VkPipelineLayout PSOCompute::GetPipelineLayout() const
{
    return m_pipelineLayout;
}

const Shader* PSOCompute::GetShader() const
{
    return m_shader;
}

const PSOCompute* PSOCompute::Get(const Shader* shader)
{
    ProfileFunction();

    auto it = s_psoCache.find((void*)shader);
    if (it != s_psoCache.end())
    {
        return it->second.get();
    }

    std::unique_ptr<PSOCompute> pso = std::make_unique<PSOCompute>(shader);

    if (pso->m_pipeline)
    {
        const PSOCompute* retValue = pso.get();
        s_psoCache.emplace((void*)shader, std::move(pso));
        return retValue;
    }

    return nullptr;
}

void PSOCompute::RecreateOnShaderChanged(const std::unordered_set<const Shader*>& changedShaders)
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

void PSOCompute::DestroyCache()
{
    s_psoCache.clear();
}

void PSOCompute::CreatePipeline()
{
    VkComputePipelineCreateInfo pipelineCreateInfo{ .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineCreateInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    pipelineCreateInfo.stage = m_shader->GetComputeStage()->GetVkShader()->GetPipelineStageCreateInfo();
    pipelineCreateInfo.layout = DescriptorBindingTable::Get()->GetPipelineLayout();

    VK_VALIDATE(vkCreateComputePipelines(VkContext::Get()->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline));
}