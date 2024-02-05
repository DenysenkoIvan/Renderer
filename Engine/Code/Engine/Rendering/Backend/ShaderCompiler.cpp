#include "ShaderCompiler.h"

#include <unordered_set>

#include <d3d12shader.h>

static std::string WCharPtrToString(const wchar_t* string)
{
    std::string result;

    int i = 0;
    const char* data = (const char*)string;
    while (data[i] != 0)
    {
        if (data[i + 1] != 0)
        {
            __debugbreak();
        }

        result += data[i];
        i += 2;
    }

    return result;
}

class IncludeHandler
    : public IDxcIncludeHandler
{
public:
    IncludeHandler(ShaderLoader& hlslFileLoader, const std::string& shaderPath)
        : m_hlslFileLoader(hlslFileLoader)
    {
        size_t offset = shaderPath.find_last_of('/', std::string::npos);

        if (offset == std::string::npos)
        {
            return;
        }

        m_directory = std::string(shaderPath.begin(), shaderPath.begin() + offset + 1);
    }

    HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
    {
        std::string filename = m_directory + WCharPtrToString(pFilename + 2);

        // If file was already included, return empty blob
        if (m_includedFiles.contains(filename))
        {
            *ppIncludeSource = nullptr;
            return S_OK;
        }
        m_includedFiles.insert(filename);

        IDxcBlob* hlslFile = m_hlslFileLoader.GetHLSLFile(filename);

        *ppIncludeSource = hlslFile;

        return S_OK;
    }

    HRESULT QueryInterface(REFIID riid, void** ppvObject)
    {
        return S_FALSE;
    }

    std::unordered_set<std::string> GetFileDependencies()
    {
        return std::move(m_includedFiles);
    }

    ULONG AddRef() override { return 0; }
    ULONG Release() override { return 0; }

private:
    std::string m_directory;
    ShaderLoader& m_hlslFileLoader;
    std::unordered_set<std::string> m_includedFiles;
};

void ShaderLoader::Create(IDxcUtils* utils)
{
    Assert(utils);

    m_utils = utils;
}

IDxcBlob* ShaderLoader::GetHLSLFile(const std::string& shaderPath)
{
    if (m_sources.contains(shaderPath))
    {
        IDxcBlob* parentBlob = m_sources[shaderPath].Get();

        IDxcBlob* result = nullptr;
        m_utils->CreateBlobFromBlob(parentBlob, 0, (UINT32)parentBlob->GetBufferSize(), &result);
        return result;
    }

    std::wstring shaderPathW(shaderPath.begin(), shaderPath.end());

    IDxcBlobEncoding* hlslBlob = nullptr;
    HRESULT hr = m_utils->LoadFile(shaderPathW.c_str(), 0, &hlslBlob);
    int i = 0;
    for (i = 0; i < 1000; i++)
    {
        if (hr != 0x80070020 || hlslBlob)
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        hr = m_utils->LoadFile(shaderPathW.c_str(), 0, &hlslBlob);
    }

    if (FAILED(hr) || !hlslBlob)
    {
        return nullptr;
    }

    m_sources.emplace(shaderPath, hlslBlob);

    return hlslBlob;
}

void ShaderLoader::ReloadShaders(const std::unordered_set<std::string>& changedShaders)
{
    for (const std::string& changedShader : changedShaders)
    {
        m_sources.erase(changedShader);
    }
}

ShaderCompiler::ShaderCompiler()
{
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.GetAddressOf()));
    Assert(m_utils);

    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.GetAddressOf()));
    Assert(m_compiler);

    m_hlslFileLoader.Create(m_utils.Get());
}

#include <stdio.h>

std::vector<uint8_t> ShaderCompiler::CompileToSpirv(const std::string& shaderPath, ShaderStageFlags stage, std::string_view entry, const ShaderDefines& defines, std::unordered_set<std::string>& dependencies)
{
    ProfileFunction();

    IDxcBlob* hlslBlob = m_hlslFileLoader.GetHLSLFile(shaderPath);

    if (!hlslBlob)
    {
        LogError("Hlsl not loaded");
        return {};
    }

    std::wstring entryW(entry.begin(), entry.end());
    std::vector<LPCWSTR> arguments
    {
        L"-E", entryW.data(),
        L"-O3",
        L"-spirv",
        L"-fvk-use-dx-layout",
        L"-fvk-use-scalar-layout",
        L"-fspv-target-env=vulkan1.3",
#if defined(DEBUG_BUILD)
        //L"-fspv-debug=vulkan-with-source",
#endif
        L"-enable-16bit-types",
        L"-T"
    };

    if (stage == ShaderStageTask)
    {
        arguments.push_back(L"as_6_7");
    }
    else if (stage == ShaderStageMesh)
    {
        arguments.push_back(L"ms_6_7");
    }
    else if (stage == ShaderStageVertex)
    {
        arguments.push_back(L"vs_6_7");
        arguments.push_back(L"-fvk-invert-y");
    }
    else if (stage == ShaderStagePixel)
    {
        arguments.push_back(L"ps_6_7");
    }
    else if (stage == ShaderStageCompute)
    {
        arguments.push_back(L"cs_6_7");
    }
    else
    {
        Assert(false, "Shader stage not supported!");
    }

    std::vector<std::wstring> definesW(defines.Get().size());
    for (const std::string& define : defines.Get())
    {
        definesW.emplace_back(define.begin(), define.end());
        arguments.push_back(L"-D");
        arguments.push_back(definesW.back().c_str());
    }

    DxcBuffer hlslBuffer{};
    hlslBuffer.Ptr = hlslBlob->GetBufferPointer();
    hlslBuffer.Size = hlslBlob->GetBufferSize();
    hlslBuffer.Encoding = DXC_CP_UTF8;

    IncludeHandler includeHandler(m_hlslFileLoader, shaderPath);

    ComPtr<IDxcResult> compileResult;
    HRESULT hr = m_compiler->Compile(&hlslBuffer, arguments.data(), (UINT32)arguments.size(), &includeHandler, IID_PPV_ARGS(compileResult.GetAddressOf()));

    ComPtr<IDxcBlobUtf8> errors;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);

    if (FAILED(hr) || (errors && errors->GetStringLength()))
    {
        LogAlways("Compilation error {} for '{}:{}'", hr, shaderPath, entry);
        LogAlways("{}", (char*)errors->GetBufferPointer());
    }

    HRESULT status = 0;
    compileResult->GetStatus(&status);
    if (FAILED(status))
    {
        LogAlways("Failed to compile shader {}", shaderPath);
        return {};
    }

    ComPtr<IDxcBlob> spirvBlob;
    compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(spirvBlob.GetAddressOf()), nullptr);

    std::vector<uint8_t> spirvCode(spirvBlob->GetBufferSize());
    memcpy(spirvCode.data(), spirvBlob->GetBufferPointer(), spirvBlob->GetBufferSize());

    auto newDependencies = includeHandler.GetFileDependencies();
    dependencies.insert(newDependencies.begin(), newDependencies.end());

    return spirvCode;
}

void ShaderCompiler::ReloadShaders(const std::unordered_set<std::string>& changedShaders)
{
    m_hlslFileLoader.ReloadShaders(changedShaders);
}