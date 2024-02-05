#pragma once

#include "RendererCommon.h"

#include "Backend/CommandBuffer.h"
#include "Backend/Shader.h"

class HDRPostProcessRenderer
{
public:
    HDRPostProcessRenderer() = default;
    ~HDRPostProcessRenderer() = default;

    void Create(const CommonRenderResources* commonResources, const RendererProperties* props);

    void RenderBloom(CommandBufferPtr& cmdBuffer, float threshold);

    const RenderStats& GetStats() const;

private:
    void BloomDownsample(CommandBufferPtr& cmdBuffer, TexturePtr& src, TexturePtr& dst, float threshold);
    void Blur(CommandBufferPtr& cmdBuffer, TexturePtr& dst, TexturePtr& temp);
    void BloomUpsample(CommandBufferPtr& cmdBuffer, TexturePtr& src, TexturePtr& dst);

private:
    const CommonRenderResources* m_commonResources = nullptr;
    const RendererProperties* m_props = nullptr;

    RenderStats m_stats{};
};