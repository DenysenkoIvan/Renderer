#include "VulkanPhysicalDevice.h"

#include "VkContext.h"
#include "VulkanValidation.h"

#include <Framework/Common.h>

void VulkanPhysicalDevice::Create(VkInstance instance)
{
    GatherExtensions();
    PickPhysicalDevice(instance);

    RetrieveFeatures();
}

VkPhysicalDevice VulkanPhysicalDevice::GetVkPhysicalDevice() const
{
    return m_physicalDevice;
}

const VkPhysicalDeviceProperties& VulkanPhysicalDevice::GetProperties() const
{
    Assert(m_physicalDevice, "Pick physical device first before querying its properties");

    return m_properties.properties;
}

const VkPhysicalDeviceDescriptorBufferPropertiesEXT& VulkanPhysicalDevice::GetDescriptorBufferProperties() const
{
    Assert(m_physicalDevice, "Pick physical device first before querying its properties");

    return m_descriptorBufferProps;
}

const VkPhysicalDeviceFeatures& VulkanPhysicalDevice::GetFeatures() const
{
    Assert(m_physicalDevice, "Pick physical device first before querying its features");

    return m_featuresCore;
}

const VkPhysicalDeviceVulkan11Features& VulkanPhysicalDevice::GetFeatures11() const
{
    Assert(m_physicalDevice, "Pick physical device first before querying its features");

    return m_features11;
}

const VkPhysicalDeviceVulkan12Features& VulkanPhysicalDevice::GetFeatures12() const
{
    Assert(m_physicalDevice, "Pick physical device first before querying its features");

    return m_features12;
}

const VkPhysicalDeviceVulkan13Features& VulkanPhysicalDevice::GetFeatures13() const
{
    Assert(m_physicalDevice, "Pick physical device first before querying its features");

    return m_features13;
}

const std::vector<const char*>& VulkanPhysicalDevice::GetExtensions()
{
    Assert(m_physicalDevice, "No appropriate physical device found");

    return m_extensions;
}

void VulkanPhysicalDevice::GatherExtensions()
{
    m_extensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,
        VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME
    };
}

void VulkanPhysicalDevice::PickPhysicalDevice(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProps{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };

        VkPhysicalDeviceProperties2 props{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        props.pNext = &descriptorBufferProps;
        vkGetPhysicalDeviceProperties2(device, &props);

        if (props.properties.apiVersion < VK_API_VERSION_1_3)
        {
            continue;
        }

        uint32_t extensionsCount = 0;
        vkEnumerateDeviceExtensionProperties(device, "", &extensionsCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
        vkEnumerateDeviceExtensionProperties(device, "", &extensionsCount, availableExtensions.data());

        Log("GPU discovered: {}", props.properties.deviceName);
        //Log("Available extensions:");
        //for (const auto& extProps : availableExtensions)
        //{
        //    Log(extProps.extensionName);
        //}

        bool extensionsSupported = true;
        for (const char* requiredExtension : m_extensions)
        {
            bool extensionSupported = false;
            for (const VkExtensionProperties& availableExtension : availableExtensions)
            {
                if (!strcmp(requiredExtension, availableExtension.extensionName))
                {
                    extensionSupported = true;
                    break;
                }
            }

            if (!extensionSupported)
            {
                extensionsSupported = false;
                break;
            }
        }

        if (!extensionsSupported)
        {
            continue;
        }

        m_physicalDevice = device;
        m_properties = props;
        m_descriptorBufferProps = descriptorBufferProps;
    }

    Assert(m_physicalDevice, "No appropriate GPU found");
}

void VulkanPhysicalDevice::RetrieveFeatures()
{
    VkPhysicalDeviceFeatures2 features2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features2.pNext = &m_features11;

    m_features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_features11.pNext = &m_features12;

    m_features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_features12.pNext = &m_features13;

    m_features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    m_features13.pNext = nullptr;

    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);

    m_featuresCore = features2.features;
}