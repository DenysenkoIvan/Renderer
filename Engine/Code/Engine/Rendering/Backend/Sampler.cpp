#include "Sampler.h"

#include "DescriptorBindingTable.h"

std::unordered_map<uint32_t, SamplerInstance> Sampler::s_states;

Sampler::Sampler(uint32_t hash)
    : m_hash(hash) {}

bool Sampler::operator==(const Sampler& sampler) const
{
    if (m_hash == -1 || sampler.m_hash == -1)
    {
        return false;
    }

    return m_hash == sampler.m_hash;
}

SamplerState Sampler::GetState()
{
    Assert(m_hash != -1);

    return s_states[m_hash].state;
}

int Sampler::GetBindSlot()
{
    if (m_hash == -1)
    {
        LogError("Invalid sampler hash value");
        return 0;
    }

    return s_states[m_hash].bindSlot;
}

VkSampler Sampler::GetVkSampler()
{
    if (m_hash == -1)
    {
        LogError("Invalid sampler hash value");
        return VK_NULL_HANDLE;
    }

    return s_states[m_hash].sampler;
}

Sampler Sampler::Get(const SamplerState& state)
{
    uint32_t hash = state.CalculateHash();

    if (s_states.contains(hash))
    {
        return hash;
    }

    SamplerInstance& samplerInstance = s_states[hash];

    samplerInstance.state = state;
    samplerInstance.sampler = CreateVkSampler(state);

    uint8_t descriptor[64]{};

    GetDescriptor(samplerInstance.sampler, descriptor);

    samplerInstance.bindSlot = DescriptorBindingTable::Get()->GetSamplerFreeSlot(descriptor);

    return hash;
}

Sampler Sampler::GetNearest()
{
    SamplerState samplerState{};
    samplerState.magFilter = FilterMode::Nearest;
    samplerState.minFilter = FilterMode::Nearest;
    
    return Get(samplerState);
}

Sampler Sampler::GetLinear()
{
    SamplerState samplerState{};
    samplerState.magFilter = FilterMode::Linear;
    samplerState.minFilter = FilterMode::Linear;
    samplerState.addressModeU = AddressMode::ClampToEdge;
    samplerState.addressModeV = AddressMode::ClampToEdge;
    samplerState.addressModeW = AddressMode::ClampToEdge;

    return Get(samplerState);
}

Sampler Sampler::GetLinearAnisotropy()
{
    SamplerState samplerState{};
    samplerState.magFilter = FilterMode::Linear;
    samplerState.minFilter = FilterMode::Linear;
    samplerState.mipmapMode = MipmapMode::Linear;
    samplerState.anisotropyEnable = true;
    samplerState.maxAnisotropy = 16.0f;

    return Get(samplerState);
}

void Sampler::DestroySamplers()
{
    for (auto it = s_states.begin(); it != s_states.end(); it++)
    {
        vkDestroySampler(VkContext::Get()->GetVkDevice(), it->second.sampler, nullptr);
    }

    s_states.clear();
}

VkSampler Sampler::CreateVkSampler(const SamplerState& state)
{
    VkSamplerCreateInfo samplerInfo{ .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = (VkFilter)state.magFilter;
    samplerInfo.minFilter = (VkFilter)state.minFilter;
    samplerInfo.mipmapMode = (VkSamplerMipmapMode)state.mipmapMode;
    samplerInfo.addressModeU = (VkSamplerAddressMode)state.addressModeU;
    samplerInfo.addressModeV = (VkSamplerAddressMode)state.addressModeV;
    samplerInfo.addressModeW = (VkSamplerAddressMode)state.addressModeW;
    samplerInfo.mipLodBias = state.mipLodBias;
    samplerInfo.anisotropyEnable = state.anisotropyEnable;
    samplerInfo.maxAnisotropy = state.maxAnisotropy;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = state.minLod;
    samplerInfo.maxLod = state.maxLod;
    samplerInfo.borderColor = (VkBorderColor)state.borderColor;
    samplerInfo.unnormalizedCoordinates = false;

    VkSampler sampler = VK_NULL_HANDLE;
    VK_VALIDATE(vkCreateSampler(VkContext::Get()->GetVkDevice(), &samplerInfo, nullptr, &sampler));

    return sampler;
}

void Sampler::GetDescriptor(VkSampler sampler, void* descriptor)
{
    VkDescriptorGetInfoEXT info{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
    info.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    info.data.pSampler = &sampler;

    VkContext::Get()->GetProcAddresses().vkGetDescriptor(VkContext::Get()->GetVkDevice(), &info, DescriptorBindingTable::Get()->GetSamplerDescriptorSize(), descriptor);
}