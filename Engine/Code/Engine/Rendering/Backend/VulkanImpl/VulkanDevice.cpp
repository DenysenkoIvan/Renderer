#include "VulkanDevice.h"

#include "VkContext.h"
#include "VulkanValidation.h"

#include <Framework/Common.h>

void VulkanDevice::Create(VulkanPhysicalDevice& physicalDevice, VkSurfaceKHR surface)
{
    GatherQueues(physicalDevice.GetVkPhysicalDevice(), surface);
    GatherEnabledFeatures(physicalDevice);

    Assert(m_graphicsQueueIndex != -1, "Failed to find graphics queue");
    Assert(m_presentQueueIndex != -1, "Failed to find present queue");

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    queueCreateInfos.push_back(CreateQueueCreateInfo(m_graphicsQueueIndex));
    if (m_presentQueue != m_graphicsQueue)
    {
        queueCreateInfos.push_back(CreateQueueCreateInfo(m_presentQueueIndex));
    }

    float queuePriority = 1.0f;
    for (auto& queueCreateInfo : queueCreateInfos)
    {
        queueCreateInfo.pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexInputFeature{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT };
    dynamicVertexInputFeature.vertexInputDynamicState = VK_TRUE;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeature{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    meshShaderFeature.taskShader = VK_TRUE;
    meshShaderFeature.meshShader = VK_TRUE;
    meshShaderFeature.multiviewMeshShader = VK_TRUE;
    meshShaderFeature.primitiveFragmentShadingRateMeshShader = VK_TRUE;
    meshShaderFeature.meshShaderQueries = VK_TRUE;

    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeature{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
    descriptorBufferFeature.descriptorBuffer = VK_TRUE;

    m_enabledFeatures.pNext = &m_enabledFeatures11;
    m_enabledFeatures11.pNext = &m_enabledFeatures12;
    m_enabledFeatures12.pNext = &m_enabledFeatures13;
    m_enabledFeatures13.pNext = &dynamicVertexInputFeature;
    dynamicVertexInputFeature.pNext = &meshShaderFeature;
    meshShaderFeature.pNext = &descriptorBufferFeature;

    VkDeviceCreateInfo deviceCreateInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCreateInfo.pNext = &m_enabledFeatures;
    deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = (uint32_t)physicalDevice.GetExtensions().size();
    deviceCreateInfo.ppEnabledExtensionNames = physicalDevice.GetExtensions().data();

    VK_VALIDATE(vkCreateDevice(physicalDevice.GetVkPhysicalDevice(), &deviceCreateInfo, nullptr, &m_device));

    vkGetDeviceQueue(m_device, m_graphicsQueueIndex, 0, &m_graphicsQueue);
    if (m_graphicsQueueIndex == m_presentQueueIndex)
    {
        m_presentQueue = m_graphicsQueue;
    }
    else
    {
        vkGetDeviceQueue(m_device, m_presentQueueIndex, 0, &m_presentQueue);
    }
}

void VulkanDevice::Destroy()
{
    vkDestroyDevice(m_device, nullptr);
}

VkDevice VulkanDevice::GetVkDevice() const
{
    Error(m_device);

    return m_device;
}

uint32_t VulkanDevice::GetGraphicsQueueIndex() const
{
    Assert(m_graphicsQueueIndex != -1);

    return m_graphicsQueueIndex;
}

uint32_t VulkanDevice::GetPresentQueueIndex() const
{
    Assert(m_presentQueueIndex != -1);

    return m_presentQueueIndex;
}

VkQueue VulkanDevice::GetGraphicsQueue() const
{
    Assert(m_device && m_graphicsQueue);

    return m_graphicsQueue;
}

VkQueue VulkanDevice::GetPresentQueue() const
{
    Assert(m_device && m_presentQueue);

    return m_presentQueue;
}

const VkPhysicalDeviceFeatures& VulkanDevice::GetEnabledFeatures() const
{
    Assert(m_device, "Querying enabled featuer before device creation");

    return m_enabledFeatures.features;
}

const VkPhysicalDeviceVulkan13Features& VulkanDevice::GetEnabledFeatures13() const
{
    Assert(m_device, "Querying enabled featuer before device creation");

    return m_enabledFeatures13;
}

void VulkanDevice::WaitIdle() const
{
    Assert(m_device);

    VK_VALIDATE(vkDeviceWaitIdle(m_device));
}

void VulkanDevice::GatherQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    uint32_t familyPropertiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyPropertiesCount, nullptr);
    std::vector<VkQueueFamilyProperties> familyProperties(familyPropertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyPropertiesCount, familyProperties.data());

    int index = 0;
    for (const VkQueueFamilyProperties& props : familyProperties)
    {
        if (m_graphicsQueueIndex != -1 && m_presentQueueIndex != -1)
        {
            break;
        }

        if (m_graphicsQueueIndex == -1 && props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_graphicsQueueIndex = index;
        }
        if (m_presentQueueIndex == -1)
        {
            VkBool32 hasSurfaceSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &hasSurfaceSupport);

            if (hasSurfaceSupport)
            {
                m_presentQueueIndex = index;
            }
        }

        index++;
    }
}

void VulkanDevice::GatherEnabledFeatures(VulkanPhysicalDevice& physicalDevice)
{
    m_enabledFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_enabledFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_enabledFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_enabledFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    const VkPhysicalDeviceFeatures& supportedFeatures = physicalDevice.GetFeatures();
    const VkPhysicalDeviceVulkan11Features& supportedFeatures11 = physicalDevice.GetFeatures11();
    const VkPhysicalDeviceVulkan12Features& supportedFeatures12 = physicalDevice.GetFeatures12();
    const VkPhysicalDeviceVulkan13Features& supportedFeatures13 = physicalDevice.GetFeatures13();

    m_enabledFeatures.features.samplerAnisotropy = supportedFeatures.samplerAnisotropy;
    m_enabledFeatures.features.wideLines = supportedFeatures.wideLines;
    m_enabledFeatures.features.independentBlend = supportedFeatures.independentBlend;
    m_enabledFeatures.features.pipelineStatisticsQuery = supportedFeatures.pipelineStatisticsQuery;
    m_enabledFeatures.features.shaderInt16 = supportedFeatures.shaderInt16;
    m_enabledFeatures11.storageBuffer16BitAccess = supportedFeatures11.storageBuffer16BitAccess;
    m_enabledFeatures11.uniformAndStorageBuffer16BitAccess = supportedFeatures11.uniformAndStorageBuffer16BitAccess;
    m_enabledFeatures11.storageInputOutput16 = supportedFeatures11.storageInputOutput16;
    m_enabledFeatures12.shaderFloat16 = supportedFeatures12.shaderFloat16;
    m_enabledFeatures12.shaderInt8 = supportedFeatures12.shaderInt8;
    m_enabledFeatures12.hostQueryReset = supportedFeatures12.hostQueryReset;
    m_enabledFeatures12.descriptorIndexing = supportedFeatures12.descriptorIndexing;
    m_enabledFeatures12.shaderSampledImageArrayNonUniformIndexing = supportedFeatures12.shaderSampledImageArrayNonUniformIndexing;
    m_enabledFeatures12.shaderStorageBufferArrayNonUniformIndexing = supportedFeatures12.shaderStorageBufferArrayNonUniformIndexing;
    m_enabledFeatures12.shaderStorageImageArrayNonUniformIndexing = supportedFeatures12.shaderInputAttachmentArrayDynamicIndexing;
    m_enabledFeatures12.descriptorBindingUpdateUnusedWhilePending = supportedFeatures12.descriptorBindingUpdateUnusedWhilePending;
    m_enabledFeatures12.descriptorBindingPartiallyBound = supportedFeatures12.descriptorBindingPartiallyBound;
    m_enabledFeatures12.descriptorBindingVariableDescriptorCount = supportedFeatures12.descriptorBindingVariableDescriptorCount;
    m_enabledFeatures12.runtimeDescriptorArray = supportedFeatures12.runtimeDescriptorArray;
    m_enabledFeatures12.scalarBlockLayout = supportedFeatures12.scalarBlockLayout;
    m_enabledFeatures12.bufferDeviceAddress = supportedFeatures12.bufferDeviceAddress;
    m_enabledFeatures13.dynamicRendering = supportedFeatures13.dynamicRendering;
    m_enabledFeatures13.synchronization2 = supportedFeatures13.synchronization2;
    m_enabledFeatures13.maintenance4 = supportedFeatures13.maintenance4;
    m_enabledFeatures13.shaderDemoteToHelperInvocation = supportedFeatures13.shaderDemoteToHelperInvocation;
}

VkDeviceQueueCreateInfo VulkanDevice::CreateQueueCreateInfo(uint32_t queue) const
{
    VkDeviceQueueCreateInfo queueCreateInfo{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueCreateInfo.queueFamilyIndex = queue;
    queueCreateInfo.queueCount = 1;

    return queueCreateInfo;
}