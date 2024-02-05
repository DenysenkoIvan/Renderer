#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum ShaderStageFlagBits
{
    ShaderStageNone   = 0,
    ShaderStageVertex = 1,
    ShaderStagePixel  = 16,
    ShaderStageCompute = 32,
    ShaderStageTask = 64,
    ShaderStageMesh = 128,

    ShaderStageAll = ShaderStageVertex | ShaderStagePixel | ShaderStageCompute | ShaderStageTask | ShaderStageMesh
};
using ShaderStageFlags = uint32_t;

enum class DescriptorType : uint32_t
{
    Sampler              = 0,
    SampledImage         = 2,
    StorageImage         = 3,
    UniformBuffer        = 6,
    StorageBuffer        = 7
};

bool IsDescriptorTypeBuffer(DescriptorType type);
bool IsDescriptorTypeTexture(DescriptorType type);

class ShaderDefines
{
public:
    ShaderDefines() = default;

    void Add(std::string_view define);

    const std::vector<std::string>& Get() const;

    bool operator==(const ShaderDefines& other) const;
    bool operator!=(const ShaderDefines& other) const;

private:
    std::vector<std::string> m_defines;
};