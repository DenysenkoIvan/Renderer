#pragma once

#include "RendererCommon.h"

#include "Backend/CommandBuffer.h"
#include "Backend/Shader.h"

class SwapchainRenderer
{
public:
    SwapchainRenderer() = default;
    ~SwapchainRenderer() = default;

    void Create(const CommonRenderResources* commonResources, const RendererProperties* props);

    void NewFrame();

    void Render(CommandBufferPtr& cmdBuffer);

    const RenderStats& GetStats() const;

private:
    void CreatePso(CommandBufferPtr& cmdBuffer);

private:
    const CommonRenderResources* m_commonResources = nullptr;
    const RendererProperties* m_rendererProps = nullptr;

    const PSOGraphics* m_pso = nullptr;

    RenderStats m_stats{};
};