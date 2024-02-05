#pragma once

#include <unordered_map>

#include <Framework/Hash.h>

#include "VulkanImpl/VkContext.h"
#include "VulkanImpl/VulkanValidation.h"

enum class FilterMode : uint8_t
{
    Nearest,
    Linear
};

enum class MipmapMode : uint8_t
{
    Nearest,
    Linear
};

enum class AddressMode : uint8_t
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class BorderColor : uint8_t
{
    FloatTransparentBlack,
    IntTransparentBlack,
    FloatOpaqueBlack,
    IntOpaqueBlack,
    FloatTransparentWhite,
    IntTransparentWhite,
    FloatOpaqueWhite,
    IntOpaqueWhite
};

struct SamplerState
{
    FilterMode magFilter = FilterMode::Nearest;
    FilterMode minFilter = FilterMode::Nearest;
    MipmapMode mipmapMode = MipmapMode::Nearest;
    AddressMode addressModeU = AddressMode::Repeat;
    AddressMode addressModeV = AddressMode::Repeat;
    AddressMode addressModeW = AddressMode::Repeat;
    float mipLodBias = 0.0f;
    bool anisotropyEnable = false;
    float maxAnisotropy = 1.0f;
    float minLod = 0.0f;
    float maxLod = 1000.0f;
    BorderColor borderColor = BorderColor::FloatTransparentBlack;

    inline uint32_t CalculateHash() const
    {
        return JenkinsHash(this, sizeof(SamplerState));
    }
};

struct SamplerInstance
{
    SamplerState state{};
    VkSampler sampler = VK_NULL_HANDLE;
    int bindSlot = -1;
};

class Sampler
{
public:
    Sampler(uint32_t hash = -1);

    bool operator==(const Sampler& sampler) const;

    SamplerState GetState();

    int GetBindSlot();

    VkSampler GetVkSampler();

    static Sampler Get(const SamplerState& state = {});
    static Sampler GetNearest();
    static Sampler GetLinear();
    static Sampler GetLinearAnisotropy();

    static void DestroySamplers();

private:
    static VkSampler CreateVkSampler(const SamplerState& state);
    static void GetDescriptor(VkSampler sampler, void* descriptor);

private:
    uint32_t m_hash = -1;

    static std::unordered_map<uint32_t, SamplerInstance> s_states;
};