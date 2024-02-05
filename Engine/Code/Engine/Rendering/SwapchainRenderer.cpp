#include "SwapchainRenderer.h"

#include "Backend/DescriptorBindingTable.h"

void SwapchainRenderer::Create(const CommonRenderResources* commonResources, const RendererProperties* props)
{
    m_commonResources = commonResources;
    m_rendererProps = props;
}

void SwapchainRenderer::NewFrame()
{
    m_stats.Reset();
}

void SwapchainRenderer::Render(CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    cmdBuffer->ResetBindAndRenderStates();

    if (!m_pso)
    {
        CreatePso(cmdBuffer);

        if (!m_pso)
        {
            return;
        }
    }

    cmdBuffer->MarkerBegin("HDR_TO_SDR");
    cmdBuffer->BeginZone("HDR_TO_SDR");

    cmdBuffer->SetViewport(0.0f, 0.0f, (float)m_rendererProps->swapchainResolution.x, (float)m_rendererProps->swapchainResolution.y, 0.0f, 1.0f);
    cmdBuffer->SetScissor(0, 0, m_rendererProps->swapchainResolution.x, m_rendererProps->swapchainResolution.y);

    cmdBuffer->BindPsoGraphics(m_pso);

    struct DrawData
    {
        int hdrTexture;
        int hdrSampler;
        int bloomTexture;
        int bloomSampler;
    };

    DrawData drawData{};
    drawData.hdrTexture = m_commonResources->hdrTarget->BindSRV();
    drawData.hdrSampler = m_rendererProps->renderResolution == m_rendererProps->swapchainResolution ?
        drawData.hdrSampler = Sampler::GetNearest().GetBindSlot() :
        drawData.hdrSampler = Sampler::GetLinear().GetBindSlot();
    drawData.bloomTexture = m_commonResources->bloomTextures[0]->BindSRV();
    drawData.bloomSampler = Sampler::GetLinear().GetBindSlot();

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    cmdBuffer->RegisterSRVUsageTexture(m_commonResources->hdrTarget);
    cmdBuffer->RegisterSRVUsageTexture(m_commonResources->bloomTextures[0]);

    m_stats.drawCallCount++;
    cmdBuffer->Draw(3);

    cmdBuffer->EndZone();
    cmdBuffer->MarkerEnd();
}

const RenderStats& SwapchainRenderer::GetStats() const
{
    return m_stats;
}

void SwapchainRenderer::CreatePso(CommandBufferPtr& cmdBuffer)
{
    PipelineGraphicsState state{};
    state.SetShader(Shader::GetGraphics("assets/shaders/HdrToSdr.hlsl"));

    cmdBuffer->PropagateRenderingInfo(state);

    m_pso = PSOGraphics::Get(state);
}