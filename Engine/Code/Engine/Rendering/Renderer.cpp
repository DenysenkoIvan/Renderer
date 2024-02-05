#include "Renderer.h"

#include "Backend/Shader.h"

#include <Application/Application.h>
#include <Framework/Common.h>
#include <Engine/Engine.h>

#include <glm/gtc/type_ptr.hpp>

Renderer* Renderer::s_renderer = nullptr;

Renderer::Renderer()
{
    ProfileFunction();

    m_driver = RenderDriver::Create();
    m_dbt = DescriptorBindingTable::Create();
    m_swapchain = Swapchain::Create();

    m_props.swapchainResolution = m_swapchain->GetSize();

    m_driver->BeginFrame();

    m_loadCmdBuffer = CommandBuffer::Create("LOAD RESOURCES");
    m_dbt->Bind(m_loadCmdBuffer);
    m_loadCmdBuffer->BeginZone("FRAME");
    m_loadCmdBuffer->BeginZone("LOAD");
    
    m_dbt->NewFrame(m_loadCmdBuffer);
    m_dbt->Init();

    m_loadCmdBuffer->BeginZone("INIT_DBT");
    m_loadCmdBuffer->EndZone();

    m_commonResources.perFrameBuffer = Buffer::CreateStructured(sizeof(PerFrameData), false);
    m_commonResources.perFrameBuffer->SetName("$PerFrameBuffer");

    m_commonResources.perFrameBuffer->BindSRV();

    m_commonResources.brdfLut = Texture::Create2D(TextureUsageSample | TextureUsageStorage, Format::RG16_UNORM, 512, 512);
    m_commonResources.brdfLut->SetName("$BRDFLut");

    m_shaderSourceWatch.Start("Assets/shaders/", true);
}

Renderer::~Renderer()
{
    m_driver->WaitIdle();
    Texture::ClearCache();
    m_driver->DestroyFramesData();
}

void Renderer::Initialize()
{
    CommandBufferPtr cmdBuffer = CommandBuffer::Create("INIT");
    m_dbt->Bind(cmdBuffer);

    m_cubemapRenderer.Create(&m_commonResources, &m_props, m_loadCmdBuffer);
    m_zpassRenderer.Create(&m_commonResources, &m_props);
    m_hdrPostProcessRenderer.Create(&m_commonResources, &m_props);
    m_swapchainRenderer.Create(&m_commonResources, &m_props);
    m_uiRenderer.Create(&m_props);

    m_cubemapRenderer.CreateBrdfLut(cmdBuffer, m_commonResources.brdfLut);

    m_driver->SubmitCommandBuffer(cmdBuffer);
}

void Renderer::Resize()
{
    m_driver->WaitIdle();
    m_swapchain->Resize();

    glm::uvec2 newResolution = m_swapchain->GetSize();

    bool oldResIsZero = m_props.swapchainResolution.x == 0 || m_props.swapchainResolution.y == 0;
    bool newResIsZero = newResolution.x == 0 || newResolution.y == 0;

    if (oldResIsZero != newResIsZero)
    {
        if (newResIsZero)
        {
            m_driver->OnSwapchainBecameUnrenderable();
        }
        else
        {
            m_driver->OnSwapchainBecameRenderable();
        }
    }

    m_props.swapchainResolution = newResolution;
    if (newResolution.x != 0 && newResolution.y != 0)
    {
        m_props.renderResolution = newResolution;
    }
}

CommandBufferPtr Renderer::GetLoadCmdBuffer() const
{
    return m_loadCmdBuffer;
}

void Renderer::LoadSkybox(const std::filesystem::path& path)
{
    ProfileFunction();

    TexturePtr equirectangular = Texture::LoadFromFile(path);

    m_loadCmdBuffer->RegisterSRVUsageTexture(equirectangular);

    TextureUsageFlags usage = TextureUsageSample | TextureUsageStorage;
    m_commonResources.skybox = Texture::Create2D(usage | TextureUsageTransferSrc | TextureUsageTransferDst, Format::RGBA16_SFLOAT, 2048, 2048, 6, true);
    m_commonResources.convolutedSkybox = Texture::Create2D(usage, Format::RGBA16_SFLOAT, 16, 16, 6);

    m_commonResources.skybox->SetName("$Skybox");
    m_commonResources.convolutedSkybox->SetName("$ConvolutedSkybox");

    int width = std::min(128, m_commonResources.skybox->GetWidth());
    int height = std::min(128, m_commonResources.skybox->GetHeight());

    for (int i = 0; i < m_commonResources.prefilteredCubemap.size(); i++)
    {
        TexturePtr& prefiltered = m_commonResources.prefilteredCubemap[i];

        prefiltered = Texture::Create2D(TextureUsageSample | TextureUsageStorage, Format::RGBA16_SFLOAT, width, height, 6);
        prefiltered->SetName(std::format("$PrefilteredCubemap_{}", i));

        width = std::max(8, width / 2);
        height = std::max(8, height / 2);
    }

    CommandBufferPtr cmdBuffer = CommandBuffer::Create("CUBEMAP_GENERATION");
    m_dbt->Bind(cmdBuffer);

    m_cubemapRenderer.GenerateCubemap(cmdBuffer, equirectangular, m_commonResources.skybox, m_commonResources.convolutedSkybox, m_commonResources.prefilteredCubemap);

    m_driver->SubmitCommandBuffer(cmdBuffer);
}

void Renderer::SubmitRenderObject(RenderObjectPtr renderObject)
{
    if (renderObject->material->props.alphaMode == AlphaMode::Opaque)
    {
        m_opaqueRenderObjects.push_back(std::move(renderObject));
    }
    else
    {
        m_transparentRenderObjects.push_back(std::move(renderObject));
    }
}

RendererProperties& Renderer::GetProps()
{
    return m_props;
}

const RendererStats& Renderer::GetStats() const
{
    return m_stats;
}

void Renderer::Render(const PerFrameData& perFrameData)
{
    ProfileFunction();

    if (m_swapchain->IsRenderable() && m_driver->AcquireNextImage(m_swapchain.get()))
    {
        m_props.swapchainResolution = m_swapchain->GetSize();
    }

    if (!m_commonResources.hdrTarget.get() ||
        m_props.renderResolution.x != m_commonResources.hdrTarget->GetWidth() ||
        m_props.renderResolution.y != m_commonResources.hdrTarget->GetHeight())
    {
        CreateRenderTargets();
    }

    CommandBufferPtr cmdBuffer = CommandBuffer::Create("RENDER");
    m_dbt->Bind(cmdBuffer);
    {
        cmdBuffer->BeginZone("RENDER");

        LoadFrameResources(perFrameData);

        if (m_props.isUseZPrepass)
        {
            m_zpassRenderer.RenderZPrepass(m_opaqueRenderObjects, cmdBuffer);
        }

        HDRRender(cmdBuffer);

        if (m_swapchain->IsRenderable())
        {
            SwapchainRendering(cmdBuffer);
        }

        cmdBuffer->EndZone();
    }

    m_loadCmdBuffer->EndZone(); // LOAD zone
    cmdBuffer->EndZone(); // FRAME zone
    m_driver->SubmitLoadCommandBuffer(m_loadCmdBuffer);
    m_driver->SubmitCommandBuffer(cmdBuffer);

    m_driver->EndFrame(m_swapchain.get());

    m_opaqueRenderObjects.clear();
    m_transparentRenderObjects.clear();

    HotReloadShaders();

    m_loadCmdBuffer = CommandBuffer::Create("LOAD RESOURCES");

    m_dbt->NewFrame(m_loadCmdBuffer);

    m_driver->BeginFrame();

    GatherStats();

    m_dbt->Bind(m_loadCmdBuffer);
    m_loadCmdBuffer->BeginZone("FRAME");
    m_loadCmdBuffer->BeginZone("LOAD");

    m_zpassRenderer.NewFrame();
    m_cubemapRenderer.NewFrame();
    m_swapchainRenderer.NewFrame();
    m_uiRenderer.NewFrame();
}

Renderer* Renderer::Get()
{
    Assert(s_renderer);

    return s_renderer;
}

RendererPtr Renderer::Create()
{
    Assert(!s_renderer);

    RendererPtr renderer = std::make_unique<Renderer>();

    s_renderer = renderer.get();

    return renderer;
}

void Renderer::CreateRenderTargets()
{
    int width = m_props.renderResolution.x;
    int height = m_props.renderResolution.y;

    m_commonResources.hdrTarget = Texture::Create2D(TextureUsageSample | TextureUsageStorage | TextureUsageColorRenderTarget, Format::RGBA16_SFLOAT, width, height);
    m_commonResources.hdrTarget->SetName("$HDRTarget");

    m_commonResources.depthTarget = Texture::Create2D(TextureUsageDepthRenderTarget, Format::D32_SFLOAT, width, height);
    m_commonResources.depthTarget->SetName("$DepthTarget");
    
    m_commonResources.bloomTextures.clear();
    m_commonResources.hdrTargetHalfs.clear();
    for (int i = 1; i < 20; i++)
    {
        int newWidth = width >> i;
        int newHeight = height >> i;

        int minSize = 2;
        if (newWidth < minSize || newHeight < minSize)
        {
            break;
        }

        TexturePtr bloomTexture = Texture::Create2D(TextureUsageSample | TextureUsageStorage, Format::RGBA16_SFLOAT, newWidth, newHeight);
        bloomTexture->SetName(std::format("$BloomX{}", std::pow(2, i)));

        m_commonResources.bloomTextures.push_back(std::move(bloomTexture));

        TexturePtr hdrHalfRes = Texture::Create2D(TextureUsageSample | TextureUsageStorage | TextureUsageColorRenderTarget, Format::RGBA16_SFLOAT, newWidth, newHeight);
        hdrHalfRes->SetName(std::format("$HDRTargetX{}", std::pow(2, i)));

        m_commonResources.hdrTargetHalfs.push_back(std::move(hdrHalfRes));
    }
}

void Renderer::HotReloadShaders()
{
    if (!m_shaderSourceWatch.IsChanged())
    {
        return;
    }

    std::vector<std::filesystem::path> changedFiles = m_shaderSourceWatch.RetrieveChangedFiles();
    std::unordered_set<std::string> relativePathes;

    const std::filesystem::path& rootPath = Application::Get()->GetRootPath();
    for (const auto& path : changedFiles)
    {
        relativePathes.insert(std::move(std::filesystem::relative(path, rootPath).generic_string()));
    }

    m_driver->WaitIdle();

    std::unordered_set<const Shader*> changedShaders = Shader::RecreateOnShaderChanges(relativePathes);
    PSOGraphics::RecreateOnShaderChanged(changedShaders);
    PSOCompute::RecreateOnShaderChanged(changedShaders);
}

void Renderer::LoadFrameResources(PerFrameData perFrameData)
{
    perFrameData.brdfLutTexture = m_commonResources.brdfLut ? m_commonResources.brdfLut->BindSRV() : 0;
    perFrameData.convolutedCubemapTexture = m_commonResources.convolutedSkybox ? m_commonResources.convolutedSkybox->BindSRV() : 0;
    for (int i = 0; i < m_commonResources.prefilteredCubemap.size(); i++)
    {
        perFrameData.prefilteredCubemaps[i] = m_commonResources.prefilteredCubemap[i] ? m_commonResources.prefilteredCubemap[i]->BindSRV() : 0;
    }
    perFrameData.samplerDesc = Sampler::GetLinearAnisotropy().GetBindSlot();

    m_loadCmdBuffer->CopyToBuffer(m_commonResources.perFrameBuffer, &perFrameData, sizeof(PerFrameData));
}

void Renderer::HDRRender(CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    cmdBuffer->MarkerBegin("HDR RENDER");

    cmdBuffer->ResetBindAndRenderStates();

    if (m_commonResources.skybox)
    {
        cmdBuffer->SetRenderTarget(0, m_commonResources.hdrTarget, false);
    }
    else
    {
        cmdBuffer->SetRenderTargetClear(0, m_commonResources.hdrTarget, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    }
    if (m_props.isUseZPrepass)
    {
        cmdBuffer->SetDepthTarget(m_commonResources.depthTarget);
    }
    else
    {
        cmdBuffer->SetDepthTargetClear(m_commonResources.depthTarget, 1.0f, 0);
    }
    cmdBuffer->BeginRenderPass();

    m_zpassRenderer.RenderZPass(m_opaqueRenderObjects, cmdBuffer, true);

    if (m_commonResources.skybox)
    {
        m_cubemapRenderer.Render(cmdBuffer);
    }

    m_zpassRenderer.RenderZPass(m_transparentRenderObjects, cmdBuffer, false);


    cmdBuffer->EndRenderPass();

    cmdBuffer->MarkerBegin("BLOOM");
    cmdBuffer->BeginZone("BLOOM");

    m_hdrPostProcessRenderer.RenderBloom(cmdBuffer, 1.0f);

    cmdBuffer->EndZone();
    cmdBuffer->MarkerEnd();

    cmdBuffer->MarkerEnd();
}

void Renderer::SwapchainRendering(CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    cmdBuffer->MarkerBegin("SWAPCHAIN RENDER");

    cmdBuffer->RegisterSRVUsageTexture(m_commonResources.hdrTarget);
    cmdBuffer->RegisterSRVUsageTexture(m_commonResources.bloomTextures[0]);

    TexturePtr swapchainTexture = m_swapchain->GetTexture();
    cmdBuffer->SetRenderTargetClear(0, swapchainTexture, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    cmdBuffer->BeginRenderPass();

    m_swapchainRenderer.Render(cmdBuffer);

    m_uiRenderer.Render(cmdBuffer);

    cmdBuffer->EndRenderPass();

    cmdBuffer->MarkerEnd();
}

void Renderer::GatherStats()
{
    m_stats.Reset();
    
    m_stats.stats += m_zpassRenderer.GetStats();
    m_stats.stats += m_cubemapRenderer.GetStats();
    m_stats.stats += m_hdrPostProcessRenderer.GetStats();
    m_stats.stats += m_swapchainRenderer.GetStats();
    m_stats.stats += m_uiRenderer.GetStats();

    m_stats.gpuZones = m_driver->GetGPUZones();
    m_stats.pipelineStatistics = m_driver->GetPipelineStatistics();
}