#include "VulkanShader.h"

#include "VkContext.h"
#include "VulkanValidation.h"

VulkanShader::VulkanShader(const std::vector<uint8_t>& spv, const char* entryPoint, VkShaderStageFlags stage)
{
    CreateModule(spv);

    m_stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_stageCreateInfo.stage = (VkShaderStageFlagBits)stage;
    m_stageCreateInfo.module = m_module;
    m_stageCreateInfo.pName = entryPoint;
}

VulkanShader::~VulkanShader()
{
    if (m_module)
    {
        DestroyModule();
    }
}

VkShaderModule VulkanShader::GetVkShaderModule() const
{
    return m_module;
}

VkShaderStageFlags VulkanShader::GetStage() const
{
    return m_stageCreateInfo.stage;
}

VkPipelineShaderStageCreateInfo VulkanShader::GetPipelineStageCreateInfo() const
{
    return m_stageCreateInfo;
}

void VulkanShader::CreateModule(const std::vector<uint8_t>& spv)
{
    VkShaderModuleCreateInfo moduleCreateInfo{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    moduleCreateInfo.pCode = (uint32_t*)spv.data();
    moduleCreateInfo.codeSize = spv.size();

    VK_VALIDATE(vkCreateShaderModule(VkContext::Get()->GetVkDevice(), &moduleCreateInfo, nullptr, &m_module));
}

void VulkanShader::DestroyModule()
{
    vkDestroyShaderModule(VkContext::Get()->GetVkDevice(), m_module, nullptr);
    m_module = VK_NULL_HANDLE;
}