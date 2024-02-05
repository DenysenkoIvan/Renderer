#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <vector>

#include <Framework/Common.h>

#include "VulkanImpl/VulkanShader.h"

#include "ShaderCompiler.h"

enum class ShaderType : uint8_t
{
    None = 0,
    Graphics = 1,
    Compute = 2
};

class Shader;

class ShaderStage
{
public:
    ShaderStage(std::vector<uint8_t> spvCode, const std::string& entryPoint, ShaderStageFlags stage);

    ShaderStageFlags GetShaderStageType() const;
    const std::string& GetEntryPoint() const;

    const std::vector<uint8_t>& GetSpirv() const;

    const VulkanShader* GetVkShader() const;

    static std::unique_ptr<ShaderStage> Create(std::vector<uint8_t> spvCode, const std::string& entryPoint, ShaderStageFlags stage);

private:
    ShaderStageFlags m_stageType = ShaderStageNone;
    std::string m_entryPoint;
    std::vector<uint8_t> m_spirvCode;
    VulkanShader m_vkShader;
};

class Shader;
using ShaderPtr = std::unique_ptr<Shader>;

class Shader
{
public:
    NON_COPYABLE_MOVABLE(Shader);

    Shader(const std::string& shaderName, const ShaderDefines& defines);
    ~Shader() = default;

    const std::string& GetName() const;
    ShaderType GetType() const;

    const ShaderDefines& GetDefines() const;
    const std::unordered_set<std::string>& GetDependencies() const;

    const ShaderStage* GetTaskStage() const;
    const ShaderStage* GetMeshStage() const;
    const ShaderStage* GetVertexStage() const;
    const ShaderStage* GetPixelStage() const;
    const ShaderStage* GetComputeStage() const;

    static Shader* GetGraphics(const std::string& shaderName, const ShaderDefines& defines = {}, bool isMeshShader = false);
    static Shader* GetCompute(const std::string& shaderName, const ShaderDefines& defines = {});
    static std::unordered_set<const Shader*> RecreateOnShaderChanges(const std::unordered_set<std::string>& changedShaderNames);

    static void ClearCache();

private:
    static std::unique_ptr<Shader> CompileShader(const std::string& shaderName, const ShaderDefines& defines, ShaderType type, bool isMeshShader);
    static Shader* FindShader(const std::string& shaderName, const ShaderDefines& defines, ShaderType type);

private:
    std::string m_name;
    ShaderType m_type;
    ShaderDefines m_defines;

    std::unique_ptr<ShaderStage> m_tsStage;
    std::unique_ptr<ShaderStage> m_msStage;
    std::unique_ptr<ShaderStage> m_vsStage;
    std::unique_ptr<ShaderStage> m_psStage;
    std::unique_ptr<ShaderStage> m_csStage;

    std::unordered_set<std::string> m_dependencies;

    static std::vector<ShaderPtr> s_shaderCache;
    static ShaderCompiler s_compiler;
};