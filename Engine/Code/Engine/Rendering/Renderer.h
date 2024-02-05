#pragma once

#include <chrono>

#include <Framework/FileWatch.h>

#include "Backend/DescriptorBindingTable.h"
#include "Backend/RenderDriver.h"
#include "Backend/Shader.h"
#include "Backend/Swapchain.h"

#include "CubemapRenderer.h"
#include "ZPassRenderer.h"
#include "SwapchainRenderer.h"
#include "HDRPostProcessRenderer.h"
#include "UIRenderer.h"
#include "RendererCommon.h"

#include "../Camera.h"

struct PerFrameData
{
    glm::mat4 projMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projViewMat = glm::mat4(1.0f);
    glm::mat4 invRotProjView = glm::mat4(1.0f);
    glm::vec4 cameraPosition = glm::vec4(0.0f);
    glm::vec4 lightDirection = glm::vec4(10.0f, -10.0f, -10.0f, 0.0f);
    glm::vec4 lightColorIntensity = glm::vec4(1.0f, 1.0f, 1.0f, 3.0f);
    int brdfLutTexture = 0;
    int convolutedCubemapTexture = 0;
    int prefilteredCubemaps[5]{};
    int samplerDesc = 0;
};

struct RendererStats
{
    RenderStats stats{};
    std::vector<GPUZone> gpuZones;
    std::vector<PipelineStatistics> pipelineStatistics;

    void Reset()
    {
        stats.Reset();
        gpuZones.clear();
        pipelineStatistics.clear();
    }
};

class Renderer;
using RendererPtr = std::unique_ptr<Renderer>;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Initialize();

    void Resize();

    CommandBufferPtr GetLoadCmdBuffer() const;

    void LoadSkybox(const std::filesystem::path& path);

    void SubmitRenderObject(RenderObjectPtr renderObject);

    RendererProperties& GetProps();
    const RendererStats& GetStats() const;

    void Render(const PerFrameData& perFrameData);

    static Renderer* Get();
    static RendererPtr Create();

private:
    void CreateRenderTargets();

    void HotReloadShaders();

    void LoadFrameResources(PerFrameData perFrameData);

    void HDRRender(CommandBufferPtr& cmdBuffer);
    void SwapchainRendering(CommandBufferPtr& cmdBuffer);

    void GatherStats();

private:
    RenderDriverPtr m_driver;
    DescriptorBindingTablePtr m_dbt;
    SwapchainPtr m_swapchain;
    
    RendererProperties m_props{};
    CommonRenderResources m_commonResources{};

    std::vector<RenderObjectPtr> m_opaqueRenderObjects;
    std::vector<RenderObjectPtr> m_transparentRenderObjects;

    UIRenderer m_uiRenderer;
    ZPassRenderer m_zpassRenderer;
    CubemapRenderer m_cubemapRenderer;
    HDRPostProcessRenderer m_hdrPostProcessRenderer;
    SwapchainRenderer m_swapchainRenderer;

    CommandBufferPtr m_loadCmdBuffer;

    RendererStats m_stats{};

    FileWatch m_shaderSourceWatch;

    static Renderer* s_renderer;
};