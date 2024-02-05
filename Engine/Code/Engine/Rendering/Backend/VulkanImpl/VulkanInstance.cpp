#include "VulkanInstance.h"

#include <Framework/Common.h>

#include "VulkanValidation.h"

void VulkanInstance::Create()
{
    uint32_t apiVersion = -1;
    VK_VALIDATE(vkEnumerateInstanceVersion(&apiVersion));

    Assert(apiVersion >= VK_API_VERSION_1_3, "Vulkan api version not supported");

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "Vulkan application";
    applicationInfo.applicationVersion = 1;
    applicationInfo.pEngineName = "My Engine";
    applicationInfo.engineVersion = 1;
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    GatherLayers();
    GatherValidationFeatures();
    GatherExtensions();

    VkInstanceCreateInfo instanceCreateInfo{ .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    
#if defined(VULKAN_VALIDATION_ENABLE)
    VkValidationFeaturesEXT validationFeatures
    {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = (uint32_t)m_validationFeatures.size(),
        .pEnabledValidationFeatures = m_validationFeatures.data()
    };

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = GetDebugMessengerCreateInfo();
    debugMessengerCreateInfo.pNext = &validationFeatures;

    instanceCreateInfo.pNext = &debugMessengerCreateInfo;
#endif

    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = (uint32_t)m_layers.size();
    instanceCreateInfo.ppEnabledLayerNames = m_layers.data();
    instanceCreateInfo.enabledExtensionCount = (uint32_t)m_extensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = m_extensions.data();

    VK_VALIDATE(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

#if defined(VULKAN_VALIDATION_ENABLE)
    CreateDebugMessenger();
#endif
}

void VulkanInstance::Destroy()
{
    Assert(m_instance, "Vulkan instance not present");

#if defined(VULKAN_VALIDATION_ENABLE)
    DestroyDebugMessenger();
#endif

    vkDestroyInstance(m_instance, nullptr);
}

VkInstance VulkanInstance::GetVkInstance()
{
    Error(m_instance, "Vulkan instance not created yet");

    return m_instance;
}

const std::vector<const char*>& VulkanInstance::GetEnabledLayers() const
{
    Assert(m_instance);

    return m_layers;
}

const std::vector<VkValidationFeatureEnableEXT>& VulkanInstance::GetValidationFeatures() const
{
    Assert(m_instance);

    return m_validationFeatures;
}

const std::vector<const char*>& VulkanInstance::GetExtensions() const
{
    Assert(m_instance);

    return m_extensions;
}

void VulkanInstance::GatherLayers()
{
#if defined(VULKAN_VALIDATION_ENABLE)
    m_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
}

void VulkanInstance::GatherValidationFeatures()
{
#if defined(VULKAN_VALIDATION_ENABLE)
    m_validationFeatures = 
    {
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        //VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };
#endif
}

void VulkanInstance::GatherExtensions()
{
    m_extensions =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface",
#if !defined(RELEASE_BUILD)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };
}

VkDebugUtilsMessengerCreateInfoEXT VulkanInstance::GetDebugMessengerCreateInfo() const
{
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = DebugCallback
    };

    return debugMessengerCreateInfo;
}

void VulkanInstance::CreateDebugMessenger()
{
    Assert(!m_debugMessenger);

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = GetDebugMessengerCreateInfo();

    auto createDebugMessengerFn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");

    Assert(createDebugMessengerFn, "Failed to create debug messenger");

    VK_VALIDATE(createDebugMessengerFn(m_instance, &debugMessengerInfo, nullptr, &m_debugMessenger));
}

void VulkanInstance::DestroyDebugMessenger()
{
    Assert(m_debugMessenger);

    auto destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");

    Assert(destroyDebugMessengerFn);

    destroyDebugMessengerFn(m_instance, m_debugMessenger, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    switch (pCallbackData->messageIdNumber)
    {
    case 1265104804:
    case 0x48a09f6c:
        return VK_FALSE;
    }

    Log("[Vulkan] {}", pCallbackData->pMessage);

    return VK_FALSE;
}