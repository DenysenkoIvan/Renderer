#include "Shader.h"

#include <Framework/Common.h>

#include "VulkanImpl/VkContext.h"
#include "VulkanImpl/VulkanValidation.h"

std::vector<ShaderPtr> Shader::s_shaderCache;
ShaderCompiler Shader::s_compiler;

ShaderStage::ShaderStage(std::vector<uint8_t> spvCode, const std::string& entryPoint, ShaderStageFlags stage)
    : m_spirvCode(std::move(spvCode)), m_stageType(stage), m_entryPoint(entryPoint), m_vkShader(m_spirvCode, m_entryPoint.c_str(), stage) {}

ShaderStageFlags ShaderStage::GetShaderStageType() const
{
    return m_stageType;
}

const std::string& ShaderStage::GetEntryPoint() const
{
    return m_entryPoint;
}

const std::vector<uint8_t>& ShaderStage::GetSpirv() const
{
    return m_spirvCode;
}

const VulkanShader* ShaderStage::GetVkShader() const
{
    return &m_vkShader;
}

std::unique_ptr<ShaderStage> ShaderStage::Create(std::vector<uint8_t> spvCode, const std::string& entryPoint, ShaderStageFlags stage)
{
    ProfileFunction();

    if (spvCode.empty())
    {
        return nullptr;
    }

    std::unique_ptr<ShaderStage> shaderStage = std::make_unique<ShaderStage>(spvCode, entryPoint, stage);
    if (shaderStage->GetVkShader()->GetVkShaderModule() != VK_NULL_HANDLE)
    {
        return shaderStage;
    }

    return nullptr;
}

Shader::Shader(const std::string& shaderName, const ShaderDefines& defines)
    : m_name(shaderName), m_defines(defines)
{}

const std::string& Shader::GetName() const
{
    return m_name;
}

ShaderType Shader::GetType() const
{
    return m_type;
}

const ShaderDefines& Shader::GetDefines() const
{
    return m_defines;
}

const std::unordered_set<std::string>& Shader::GetDependencies() const
{
    return m_dependencies;
}

const ShaderStage* Shader::GetTaskStage() const
{
    return m_tsStage.get();
}

const ShaderStage* Shader::GetMeshStage() const
{
    return m_msStage.get();
}

const ShaderStage* Shader::GetVertexStage() const
{
    return m_vsStage.get();
}

const ShaderStage* Shader::GetPixelStage() const
{
    return m_psStage.get();
}

const ShaderStage* Shader::GetComputeStage() const
{
    return m_csStage.get();
}

Shader* Shader::GetGraphics(const std::string& shaderName, const ShaderDefines& defines, bool isMeshShader)
{
    ProfileFunction();

    Shader* shader = FindShader(shaderName, defines, ShaderType::Graphics);
    if (shader)
    {
        return shader;
    }

    std::unique_ptr<Shader> newShader = CompileShader(shaderName, defines, ShaderType::Graphics, isMeshShader);
    if (newShader)
    {
        s_shaderCache.push_back(std::move(newShader));
        return s_shaderCache.back().get();
    }
    else
    {
        return nullptr;
    }
}

Shader* Shader::GetCompute(const std::string& shaderName, const ShaderDefines& defines)
{
    ProfileFunction();

    Shader* shader = FindShader(shaderName, defines, ShaderType::Compute);
    if (shader)
    {
        return shader;
    }

    std::unique_ptr<Shader> newShader = CompileShader(shaderName, defines, ShaderType::Compute, false);
    if (newShader)
    {
        s_shaderCache.push_back(std::move(newShader));
        return s_shaderCache.back().get();
    }
    else
    {
        return nullptr;
    }
}

std::unordered_set<const Shader*> Shader::RecreateOnShaderChanges(const std::unordered_set<std::string>& changedShaderNames)
{
    s_compiler.ReloadShaders(changedShaderNames);

    auto isSetIntersects = [](const std::unordered_set<std::string>& set1, const std::unordered_set<std::string>& set2)
        {
            for (const std::string& str : set1)
            {
                if (set2.contains(str))
                {
                    return true;
                }
            }
            return false;
        };

    std::unordered_set<const Shader*> changedShaders;

    for (ShaderPtr& shader : s_shaderCache)
    {
        if (!changedShaderNames.contains(shader->GetName()) && !isSetIntersects(shader->GetDependencies(), changedShaderNames))
        {
            continue;
        }

        ShaderPtr recompiled = CompileShader(shader->GetName(), shader->GetDefines(), shader->GetType(), shader->GetTaskStage() || shader->GetMeshStage());

        shader->m_tsStage = std::move(recompiled->m_tsStage);
        shader->m_msStage = std::move(recompiled->m_msStage);
        shader->m_vsStage = std::move(recompiled->m_vsStage);
        shader->m_psStage = std::move(recompiled->m_psStage);
        shader->m_csStage = std::move(recompiled->m_csStage);

        shader->m_dependencies = std::move(recompiled->m_dependencies);

        changedShaders.insert(shader.get());
    }

    return changedShaders;
}

void Shader::ClearCache()
{
    s_shaderCache.clear();
}

std::unique_ptr<Shader> Shader::CompileShader(const std::string& shaderName, const ShaderDefines& defines, ShaderType type, bool isMeshShader)
{
    ProfileFunction();

    const char* tsStageEntry = "MainTS";
    const char* msStageEntry = "MainMS";
    const char* vsStageEntry = "MainVS";
    const char* psStageEntry = "MainPS";
    const char* csStageEntry = "MainCS";

    std::unique_ptr<Shader> shader = std::make_unique<Shader>(shaderName, defines);

    if (type == ShaderType::Graphics)
    {
        if (isMeshShader)
        {
            std::vector<uint8_t> tsSpirv = s_compiler.CompileToSpirv(shaderName, ShaderStageTask, tsStageEntry, defines, shader->m_dependencies);
            if (!tsSpirv.empty())
            {
                shader->m_tsStage = ShaderStage::Create(std::move(tsSpirv), tsStageEntry, ShaderStageTask);
            }

            std::vector<uint8_t> msSpirv = s_compiler.CompileToSpirv(shaderName, ShaderStageMesh, msStageEntry, defines, shader->m_dependencies);
            shader->m_msStage = ShaderStage::Create(std::move(msSpirv), msStageEntry, ShaderStageMesh);
        }
        else
        {
            std::vector<uint8_t> vsSpirv = s_compiler.CompileToSpirv(shaderName, ShaderStageVertex, vsStageEntry, defines, shader->m_dependencies);
            shader->m_vsStage = ShaderStage::Create(std::move(vsSpirv), vsStageEntry, ShaderStageVertex);
        }

        std::vector<uint8_t> psSpirv = s_compiler.CompileToSpirv(shaderName, ShaderStagePixel, psStageEntry, defines, shader->m_dependencies);
        if (!psSpirv.empty())
        {
            shader->m_psStage = ShaderStage::Create(std::move(psSpirv), psStageEntry, ShaderStagePixel);
        }

        shader->m_type = ShaderType::Graphics;
    }
    else if (type == ShaderType::Compute)
    {
        std::vector<uint8_t> csSpirv = s_compiler.CompileToSpirv(shaderName, ShaderStageCompute, csStageEntry, defines, shader->m_dependencies);
        if (csSpirv.empty())
        {
            return nullptr;
        }

        shader->m_csStage = ShaderStage::Create(std::move(csSpirv), csStageEntry, ShaderStageCompute);
        shader->m_type = ShaderType::Compute;
    }
    else
    {
        Assert(false, "Invalid shader type");
    }

    return shader;
}

Shader* Shader::FindShader(const std::string& shaderName, const ShaderDefines& defines, ShaderType type)
{
    auto it = std::find_if(s_shaderCache.begin(), s_shaderCache.end(), [&](const std::unique_ptr<Shader>& shader)
        {
            if (shader->GetType() != type)
            {
                return false;
            }

            if (shader->GetName() != shaderName)
            {
                return false;
            }

            if (shader->GetDefines() != defines)
            {
                return false;
            }

            return true;
        });

    return it != s_shaderCache.end() ? it->get() : nullptr;
}