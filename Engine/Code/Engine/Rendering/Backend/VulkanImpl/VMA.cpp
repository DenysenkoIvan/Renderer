#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VMA.h"

#include "VulkanValidation.h"

VmaAllocator VMA::s_allocator = VK_NULL_HANDLE;

void VMA::Initialize()
{
    VmaVulkanFunctions vmaFunctions{};
    vmaFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vmaFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.physicalDevice = VkContext::Get()->GetVkPhysicalDevice();
    allocatorInfo.device = VkContext::Get()->GetVkDevice();
    allocatorInfo.instance = VkContext::Get()->GetVkInstance();
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VK_VALIDATE(vmaCreateAllocator(&allocatorInfo, &s_allocator));
}

void VMA::Deinitialize()
{
    vmaDestroyAllocator(s_allocator);
}

VmaAllocator VMA::Allocator()
{
    Assert(s_allocator != VK_NULL_HANDLE, "VMA not initialized yet");

    return s_allocator;
}