#include "ShaderUtils.h"

#include <algorithm>

bool IsDescriptorTypeBuffer(DescriptorType type)
{
    switch (type)
    {
    case DescriptorType::UniformBuffer:
    case DescriptorType::StorageBuffer:
        return true;
    default:
        return false;
    }
}

bool IsDescriptorTypeTexture(DescriptorType type)
{
    switch (type)
    {
    case DescriptorType::SampledImage:
    case DescriptorType::StorageImage:
        return true;
    default:
        return false;
    }
}

void ShaderDefines::Add(std::string_view define)
{
    m_defines.emplace_back(define);
    std::sort(m_defines.begin(), m_defines.end());
}

const std::vector<std::string>& ShaderDefines::Get() const
{
    return m_defines;
}

bool ShaderDefines::operator==(const ShaderDefines& other) const
{
    if (other.m_defines.size() != m_defines.size())
    {
        return false;
    }

    for (const std::string& define : m_defines)
    {
        auto otherDefineIt = std::find_if(other.m_defines.begin(), other.m_defines.end(), [&](const std::string& otherDefine)
            {
                return define == otherDefine;
            });

        if (otherDefineIt == other.m_defines.end())
        {
            return false;
        }
    }

    return true;
}

bool ShaderDefines::operator!=(const ShaderDefines& other) const
{
    return !(*this == other);
}