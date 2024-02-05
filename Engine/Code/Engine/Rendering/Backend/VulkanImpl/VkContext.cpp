#include "VkContext.h"

#include <Framework/Common.h>

VkContext* VkContext::s_context = nullptr;

void VkContext::Create()
{
    ProfileFunction();

    m_instance.Create();
    m_physicalDevice.Create(m_instance.GetVkInstance());
    m_surface.Create(m_instance.GetVkInstance(), m_physicalDevice.GetVkPhysicalDevice());
    m_device.Create(m_physicalDevice, m_surface.GetVkSurface());

    RetrieveProcAddresses();
}

void VkContext::Destroy()
{
    ProfileFunction();

    m_device.Destroy();
    m_surface.Destroy();
    m_instance.Destroy();

    ZeroMemory(&m_addresses, sizeof(m_addresses));
}

VkInstance VkContext::GetVkInstance()
{
    return m_instance.GetVkInstance();
}

VkPhysicalDevice VkContext::GetVkPhysicalDevice()
{
    return m_physicalDevice.GetVkPhysicalDevice();
}

VkDevice VkContext::GetVkDevice()
{
    return m_device.GetVkDevice();
}

VulkanInstance& VkContext::GetInstance()
{
    return m_instance;
}

VulkanPhysicalDevice& VkContext::GetPhysicalDevice()
{
    return m_physicalDevice;
}

VulkanSurface& VkContext::GetSurface()
{
    return m_surface;
}

VulkanDevice& VkContext::GetDevice()
{
    return m_device;
}

VkContextProcAddresses& VkContext::GetProcAddresses()
{
    return m_addresses;
}

VkContext* VkContext::Get()
{
    return s_context;
}

void VkContext::Set(VkContext* context)
{
    s_context = context;
}

void VkContext::RetrieveProcAddresses()
{
    VkDevice device = m_device.GetVkDevice();
    m_addresses.vkCmdSetVertexInput = (PFN_vkCmdSetVertexInputEXT)vkGetDeviceProcAddr(device, "vkCmdSetVertexInputEXT");
    m_addresses.vkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
    m_addresses.vkCmdEndDebugUtilsLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
    m_addresses.vkCmdDrawMeshTasks = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT");
    m_addresses.vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(m_instance.GetVkInstance(), "vkSetDebugUtilsObjectNameEXT");
    m_addresses.vkGetDescriptorSetLayoutSize = (PFN_vkGetDescriptorSetLayoutSizeEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSizeEXT");
    m_addresses.vkGetDescriptor = (PFN_vkGetDescriptorEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorEXT");
    m_addresses.vkGetDescriptorSetLayoutBindingOffset = (PFN_vkGetDescriptorSetLayoutBindingOffsetEXT)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutBindingOffsetEXT");
    m_addresses.vkCmdBindDescriptorBuffers = (PFN_vkCmdBindDescriptorBuffersEXT)vkGetDeviceProcAddr(device, "vkCmdBindDescriptorBuffersEXT");
    m_addresses.vkCmdSetDescriptorBufferOffsets = (PFN_vkCmdSetDescriptorBufferOffsetsEXT)vkGetDeviceProcAddr(device, "vkCmdSetDescriptorBufferOffsetsEXT");
}