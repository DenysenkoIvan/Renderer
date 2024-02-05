#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class VulkanShader
{
public:
    VulkanShader(const std::vector<uint8_t>& spv, const char* entryPoint, VkShaderStageFlags stage);
    ~VulkanShader();

    VkShaderModule GetVkShaderModule() const;
    VkShaderStageFlags GetStage() const;

    VkPipelineShaderStageCreateInfo GetPipelineStageCreateInfo() const;

private:
    void CreateModule(const std::vector<uint8_t>& spv);

    void DestroyModule();

private:
    VkShaderModule m_module = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo m_stageCreateInfo{};
};