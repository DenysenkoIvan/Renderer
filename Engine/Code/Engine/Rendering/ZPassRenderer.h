#pragma once

#include "Backend/CommandBuffer.h"

#include "RendererCommon.h"
#include "RenderObject.h"

class ZPassRenderer
{
public:
    ZPassRenderer() = default;
    ~ZPassRenderer() = default;

    void Create(const CommonRenderResources* commonResources, const RendererProperties* props);

    void NewFrame();

    void RenderZPrepass(std::vector<RenderObjectPtr>& inRenderObjects, CommandBufferPtr& cmdBuffer);
    void RenderZPass(std::vector<RenderObjectPtr>& inRenderObjects, CommandBufferPtr& cmdBuffer, bool isOpaque);

    const RenderStats& GetStats() const;

private:
    const PSOGraphics* CreateZPrePassDrawCallPSO(RenderObjectPtr& renderObject, CommandBufferPtr& cmdBuffer);
    const PSOGraphics* CreateZPrePassMeshDrawCallPSO(RenderObjectPtr& renderObject, CommandBufferPtr& cmdBuffer);
    const PSOGraphics* CreateZPassDrawCallPSO(RenderObjectPtr& renderObject, CommandBufferPtr& cmdBuffer);
    const PSOGraphics* CreateZPassMeshDrawCallPSO(RenderObjectPtr& renderObject, CommandBufferPtr& cmdBuffer);

private:
    const CommonRenderResources* m_commonResources = nullptr;
    const RendererProperties* m_renderProps = nullptr;

    RenderStats m_stats{};
};