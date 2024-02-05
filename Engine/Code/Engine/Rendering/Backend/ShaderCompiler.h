#pragma once

#include <Framework/Common.h>

#include "ShaderUtils.h"

#include <combaseapi.h>
#include <dxc/dxcapi.h>

#include <map>
#include <unordered_set>
#include <vector>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class ShaderLoader
{
public:
    void Create(IDxcUtils* utils);

    IDxcBlob* GetHLSLFile(const std::string& shaderPath);

    void ReloadShaders(const std::unordered_set<std::string>& changedShaders);

private:
    std::map<std::string, ComPtr<IDxcBlob>> m_sources;
    IDxcUtils* m_utils;
};

class ShaderCompiler
{
public:
    NON_COPYABLE_MOVABLE(ShaderCompiler);

    ShaderCompiler();

    std::vector<uint8_t> CompileToSpirv(const std::string& shaderPath, ShaderStageFlags stage, std::string_view entry, const ShaderDefines& defines, std::unordered_set<std::string>& dependencies);

    void ReloadShaders(const std::unordered_set<std::string>& changedShaders);

private:
    ShaderLoader m_hlslFileLoader;
    ComPtr<IDxcUtils> m_utils;
    ComPtr<IDxcCompiler3> m_compiler;
};