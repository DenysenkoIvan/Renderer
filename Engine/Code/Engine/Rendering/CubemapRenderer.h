#pragma once

#include <filesystem>

#include "Backend/CommandBuffer.h"

#include "RendererCommon.h"

class CubemapRenderer
{
public:
    CubemapRenderer() = default;
    ~CubemapRenderer() = default;
    
    void Create(const CommonRenderResources* commonResources, const RendererProperties* props, CommandBufferPtr& cmdBuffer);

    void NewFrame();

    void CreateBrdfLut(CommandBufferPtr& cmdBuffer, TexturePtr& brdfLut);
    void GenerateCubemap(CommandBufferPtr& cmdBuffer, TexturePtr& equirectangular, TexturePtr& cubemap, TexturePtr& convolutedCubemap, std::array<TexturePtr, 5>& prefiltered);

    void Render(CommandBufferPtr& cmdBuffer);

    const RenderStats& GetStats() const;

private:
    void CreateRenderPso(CommandBufferPtr& cmdBuffer);

    void ConvertFromEquirectangularToCubemap(CommandBufferPtr& cmdBuffer, TexturePtr& equirectangular, TexturePtr& cubemap);
    void Convolute(CommandBufferPtr& cmdBuffer, TexturePtr& cubemap, TexturePtr& convolutedCubemap);
    void Prefilter(CommandBufferPtr& cmdBuffer, TexturePtr& cubemap, std::array<TexturePtr, 5>& prefilteredLevels);

    TexturePtr LoadCubemapImage(const std::filesystem::path& path, CommandBufferPtr& cmdBuffer);

private:
    const CommonRenderResources* m_commonResources = nullptr;
    const RendererProperties* m_renderProps = nullptr;

    const PSOGraphics* m_renderCubemapPso = nullptr;

    RenderStats m_stats{};
};