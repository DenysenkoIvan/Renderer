#pragma once

#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"

struct VkContextProcAddresses
{
    PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInput = nullptr;
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabel = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabel = nullptr;
    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasks = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectName = nullptr;
    PFN_vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSize = nullptr;
    PFN_vkGetDescriptorEXT vkGetDescriptor = nullptr;
    PFN_vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffset = nullptr;
    PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffers = nullptr;
    PFN_vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsets = nullptr;
};

class VkContext
{
public:
    void Create();
    void Destroy();
    
    VkInstance GetVkInstance();
    VkPhysicalDevice GetVkPhysicalDevice();
    VkDevice GetVkDevice();

    VulkanInstance& GetInstance();
    VulkanPhysicalDevice& GetPhysicalDevice();
    VulkanSurface& GetSurface();
    VulkanDevice& GetDevice();

    VkContextProcAddresses& GetProcAddresses();

    static VkContext* Get();
    static void Set(VkContext* context);

private:
    void RetrieveProcAddresses();

private:
    VulkanInstance m_instance;
    VulkanPhysicalDevice m_physicalDevice;
    VulkanSurface m_surface;
    VulkanDevice m_device;
    VkContextProcAddresses m_addresses;

    static VkContext* s_context;
};