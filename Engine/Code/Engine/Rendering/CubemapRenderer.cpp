#include "CubemapRenderer.h"

#include "Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

void CubemapRenderer::Create(const CommonRenderResources* commonResources, const RendererProperties* props, CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    m_commonResources = commonResources;
    m_renderProps = props;
}

void CubemapRenderer::NewFrame()
{
    m_stats.Reset();
}

void CubemapRenderer::CreateBrdfLut(CommandBufferPtr& cmdBuffer, TexturePtr& brdfLut)
{
    ProfileFunction();

    cmdBuffer->MarkerBegin("BRDF_LUT_GENERATION");

    struct DrawData
    {
        int brdfLut;
        float brdfLutSizes[2];
    };
    
    DrawData drawData{};
    drawData.brdfLut = brdfLut->BindUAV();
    drawData.brdfLutSizes[0] = 1.0f / brdfLut->GetWidth();
    drawData.brdfLutSizes[1] = 1.0f / brdfLut->GetHeight();

    cmdBuffer->RegisterUAVUsageTexture(brdfLut);

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    const PSOCompute* generationPso = PSOCompute::Get(Shader::GetCompute("assets/shaders/BrdfLutGeneration.hlsl"));
    cmdBuffer->BindPsoCompute(generationPso);

    m_stats.dispatchCount++;
    cmdBuffer->Dispatch(brdfLut->GetWidth() / 64, brdfLut->GetHeight(), 1);

    cmdBuffer->MarkerEnd();
}

void CubemapRenderer::GenerateCubemap(CommandBufferPtr& cmdBuffer, TexturePtr& equirectangular, TexturePtr& cubemap, TexturePtr& convolutedCubemap, std::array<TexturePtr, 5>& prefiltered)
{
    ProfileFunction();

    cmdBuffer->BeginZone("CUBEMAP_GENERATION");
    cmdBuffer->MarkerBegin("CUBEMAP_GENERATION");

    ConvertFromEquirectangularToCubemap(cmdBuffer, equirectangular, cubemap);
    Convolute(cmdBuffer, cubemap, convolutedCubemap);
    Prefilter(cmdBuffer, cubemap, prefiltered);

    cmdBuffer->MarkerEnd();
    cmdBuffer->EndZone();
}

void CubemapRenderer::Render(CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    if (!m_renderCubemapPso)
    {
        CreateRenderPso(cmdBuffer);

        if (!m_renderCubemapPso)
        {
            LogError("Failed to create graphics PSO to render cubemap");
            return;
        }
    }

    cmdBuffer->BeginZone("CUBEMAP_RENDER");
    cmdBuffer->MarkerBegin("CUBEMAP_RENDER");

    cmdBuffer->ResetBindAndRenderStates();

    TexturePtr hdrTarget = m_commonResources->hdrTarget;
    cmdBuffer->SetViewport(0.0f, 0.0f, (float)hdrTarget->GetWidth(), (float)hdrTarget->GetHeight(), 0.0f, 1.0f);
    cmdBuffer->SetScissor(0, 0, hdrTarget->GetWidth(), hdrTarget->GetHeight());

    struct DrawData
    {
        int skybox;
        int skyboxSampler;
        int perFrameBuffer;
    };

    DrawData drawData{};
    drawData.skybox = m_commonResources->skybox->BindSRV();
    drawData.skyboxSampler = Sampler::GetLinearAnisotropy().GetBindSlot();
    drawData.perFrameBuffer = m_commonResources->perFrameBuffer->BindSRV();

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    cmdBuffer->RegisterSRVUsageTexture(m_commonResources->skybox);
    cmdBuffer->RegisterSRVUsageBuffer(m_commonResources->perFrameBuffer);

    cmdBuffer->BindPsoGraphics(m_renderCubemapPso);

    m_stats.drawCallCount++;
    cmdBuffer->Draw(36);

    cmdBuffer->MarkerEnd();
    cmdBuffer->EndZone();
}

const RenderStats& CubemapRenderer::GetStats() const
{
    return m_stats;
}

void CubemapRenderer::CreateRenderPso(CommandBufferPtr& cmdBuffer)
{
    PipelineGraphicsState state{};
    state.SetShader(Shader::GetGraphics("assets/shaders/Cubemap.hlsl"));
    state.GetDepthStencilState().SetDepthBoundsTest(0.99999f, 1.00001f);

    cmdBuffer->PropagateRenderingInfo(state);

    m_renderCubemapPso = PSOGraphics::Get(state);
}

void CubemapRenderer::ConvertFromEquirectangularToCubemap(CommandBufferPtr& cmdBuffer, TexturePtr& equirectangular, TexturePtr& cubemap)
{
    cmdBuffer->MarkerBegin("EQUIRECTANGULAR_TO_CUBEMAP");

    cmdBuffer->ResetBindAndRenderStates();

    struct DrawData
    {
        int equirectangular;
        int equirectangularSampler;
        int cubemapFaces[6];
        float cubemapSizes[2];
    };

    DrawData drawData{};
    drawData.equirectangular = equirectangular->BindSRV();
    drawData.equirectangularSampler = Sampler::GetLinearAnisotropy().GetBindSlot();
    for (int i = 0; i < 6; i++)
    {
        drawData.cubemapFaces[i] = cubemap->BindUAVLayer(i);
    }
    drawData.cubemapSizes[0] = 1.0f / cubemap->GetWidth();
    drawData.cubemapSizes[1] = 1.0f / cubemap->GetHeight();

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    cmdBuffer->RegisterSRVUsageTexture(equirectangular);
    cmdBuffer->RegisterUAVUsageTexture(cubemap);

    const PSOCompute* generationPso = PSOCompute::Get(Shader::GetCompute("assets/shaders/EquirectangularToCubemap.hlsl"));
    cmdBuffer->BindPsoCompute(generationPso);

    m_stats.dispatchCount++;
    cmdBuffer->Dispatch(cubemap->GetWidth() / 64, cubemap->GetHeight(), 6);

    cmdBuffer->GenerateMipmaps(cubemap);

    cmdBuffer->RegisterSRVUsageTexture(cubemap);

    cmdBuffer->MarkerEnd();
}

void CubemapRenderer::Convolute(CommandBufferPtr& cmdBuffer, TexturePtr& cubemap, TexturePtr& convolutedCubemap)
{
    cmdBuffer->MarkerBegin("CUBEMAP_CONVOLUTION");

    struct DrawData
    {
        int cubemap;
        int cubemapSampler;
        int convolutedCubemap[6];
        float sizeInv[2];
    };

    DrawData drawData{};
    drawData.cubemap = cubemap->BindSRV();
    drawData.cubemapSampler = Sampler::GetLinearAnisotropy().GetBindSlot();
    for (int i = 0; i < 6; i++)
    {
        drawData.convolutedCubemap[i] = convolutedCubemap->BindUAVLayer(i);
    }
    drawData.sizeInv[0] = 1.0f / convolutedCubemap->GetWidth();
    drawData.sizeInv[1] = 1.0f / convolutedCubemap->GetHeight();

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    cmdBuffer->RegisterSRVUsageTexture(cubemap);
    cmdBuffer->RegisterUAVUsageTexture(convolutedCubemap);

    const PSOCompute* convolutionPso = PSOCompute::Get(Shader::GetCompute("assets/shaders/CubemapConvolution.hlsl"));
    cmdBuffer->BindPsoCompute(convolutionPso);

    m_stats.dispatchCount++;
    cmdBuffer->Dispatch(convolutedCubemap->GetWidth(), convolutedCubemap->GetHeight(), 6);

    cmdBuffer->RegisterSRVUsageTexture(convolutedCubemap);

    cmdBuffer->MarkerEnd();
}

void CubemapRenderer::Prefilter(CommandBufferPtr& cmdBuffer, TexturePtr& cubemap, std::array<TexturePtr, 5>& prefilteredLevels)
{
    cmdBuffer->MarkerBegin("PREFILTERING");

    struct DrawData
    {
        int cubemap;
        int cubemapSampler;
        int prefilteredFaces[6];
        float prefilteredCubemapSizes[5];
    };

    const PSOCompute* prefilteringPso = PSOCompute::Get(Shader::GetCompute("assets/shaders/CubemapPrefiltering.hlsl"));

    for (int i = 0; i < prefilteredLevels.size(); i++)
    {
        TexturePtr& prefiltered = prefilteredLevels[i];

        DrawData drawData{};
        drawData.cubemap = cubemap->BindSRV();
        drawData.cubemapSampler = Sampler::GetLinearAnisotropy().GetBindSlot();
        for (int j = 0; j < 6; j++)
        {
            drawData.prefilteredFaces[j] = prefiltered->BindUAVLayer(j);
        }
        drawData.prefilteredCubemapSizes[0] = 1.0f / prefiltered->GetWidth();
        drawData.prefilteredCubemapSizes[1] = 1.0f / prefiltered->GetHeight();
        drawData.prefilteredCubemapSizes[2] = (float)cubemap->GetHeight();
        drawData.prefilteredCubemapSizes[3] = (float)cubemap->GetHeight();
        drawData.prefilteredCubemapSizes[4] = (float)i / prefilteredLevels.size();

        cmdBuffer->PushConstants(&drawData, sizeof(drawData));

        cmdBuffer->RegisterSRVUsageTexture(cubemap);
        cmdBuffer->RegisterUAVUsageTexture(prefiltered);

        cmdBuffer->BindPsoCompute(prefilteringPso);

        m_stats.dispatchCount++;
        cmdBuffer->Dispatch(prefiltered->GetWidth() / 8, prefiltered->GetHeight() / 8, 6);
    }

    cmdBuffer->MarkerEnd();
}

TexturePtr CubemapRenderer::LoadCubemapImage(const std::filesystem::path& path, CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    std::string skyboxPath = path.string();

    int width = 0;
    int height = 0;
    int channelsCount = 4;
    int desiredChannelsCount = 4;

    float* data = stbi_loadf(skyboxPath.c_str(), &width, &height, &channelsCount, desiredChannelsCount);
    if (!data)
    {
        LogError("Failed to load skybox {}", skyboxPath);
        return nullptr;
    }

    TexturePtr tmpTexture = Texture::Create2D(TextureUsageTransferDst | TextureUsageSample, Format::RGBA32_SFLOAT, width, height);

    cmdBuffer->CopyToTexture(tmpTexture, data, width * height * sizeof(float) * desiredChannelsCount);

    stbi_image_free(data);

    cmdBuffer->RegisterSRVUsageTexture(tmpTexture);

    return tmpTexture;
}