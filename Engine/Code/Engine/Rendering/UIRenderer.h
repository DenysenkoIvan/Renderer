#pragma once

#include "Backend/CommandBuffer.h"
#include "Backend/Shader.h"
#include "RendererCommon.h"

class UIRenderer
{
public:
    UIRenderer() = default;
    ~UIRenderer() = default;

    void Create(const RendererProperties* props);

    void NewFrame();

    void Render(CommandBufferPtr& cmdBuffer);

    const RenderStats& GetStats() const;

private:
    void CreatePso(CommandBufferPtr& cmdBuffer);

    bool Prepare();

    void CreateFontsTexture(CommandBufferPtr& cmdBuffer);

private:
    const RendererProperties* m_rendererProps = nullptr;

    int m_vtxCount = 0;
    int m_idxCount = 0;
    BufferPtr m_uiVtx;
    BufferPtr m_uiIdx;
    TexturePtr m_fontsTexture;
    BufferPtr m_perFrameBuffer;

    const PSOGraphics* m_pso = nullptr;

    RenderStats m_stats{};
};