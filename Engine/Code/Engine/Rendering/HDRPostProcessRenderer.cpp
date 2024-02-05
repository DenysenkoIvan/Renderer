#include "HDRPostProcessRenderer.h"

void HDRPostProcessRenderer::Create(const CommonRenderResources* commonResources, const RendererProperties* props)
{
    m_commonResources = commonResources;
    m_props = props;
}

void HDRPostProcessRenderer::RenderBloom(CommandBufferPtr& cmdBuffer, float threshold)
{
    for (int i = 0; i < m_commonResources->bloomTextures.size(); i++)
    {
        TexturePtr src = i == 0 ? m_commonResources->hdrTarget : m_commonResources->bloomTextures[i - 1];
        TexturePtr dst = m_commonResources->bloomTextures[i];

        if (i > 0)
        {
            threshold = 0.0f;
        }

        BloomDownsample(cmdBuffer, src, dst, threshold);

        TexturePtr tempTexture = m_commonResources->hdrTargetHalfs[i];
        Blur(cmdBuffer, dst, tempTexture);
    }

    for (int i = 1; i < m_commonResources->bloomTextures.size(); i++)
    {
        int index = (int)m_commonResources->bloomTextures.size() - i - 1;

        TexturePtr dst = m_commonResources->bloomTextures[index];
        TexturePtr src = m_commonResources->bloomTextures[index + 1];

        BloomUpsample(cmdBuffer, src, dst);
    }
}

const RenderStats& HDRPostProcessRenderer::GetStats() const
{
    return m_stats;
}

void HDRPostProcessRenderer::BloomDownsample(CommandBufferPtr& cmdBuffer, TexturePtr& src, TexturePtr& dst, float threshold)
{
    cmdBuffer->RegisterSRVUsageTexture(src);
    cmdBuffer->RegisterUAVUsageTexture(dst);

    struct DrawData
    {
        int bloomWidth;
        int bloomHeight;
        float bloomWidthInv;
        float bloomHeightInv;
        float hdrWidthInv;
        float hdrHeightInv;
        int hdrTexture;
        int hdrSampler;
        int bloomTexture;
        float threshold;
    };

    DrawData drawData{};
    drawData.bloomWidth = dst->GetWidth();
    drawData.bloomHeight = dst->GetHeight();
    drawData.bloomWidthInv = 1.0f / dst->GetWidth();
    drawData.bloomHeightInv = 1.0f / dst->GetHeight();
    drawData.hdrWidthInv = 1.0f / src->GetWidth();
    drawData.hdrHeightInv = 1.0f / src->GetHeight();
    drawData.hdrTexture = src->BindSRV();
    drawData.hdrSampler = Sampler::GetLinear().GetBindSlot();
    drawData.bloomTexture = dst->BindUAV();
    drawData.threshold = threshold;

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    const PSOCompute* bloomPso = PSOCompute::Get(Shader::GetCompute("assets/shaders/BloomDownsample.hlsl"));
    cmdBuffer->BindPsoCompute(bloomPso);

    cmdBuffer->Dispatch((drawData.bloomWidth + 7) / 8, (drawData.bloomHeight + 7) / 8, 1);
}

void HDRPostProcessRenderer::Blur(CommandBufferPtr& cmdBuffer, TexturePtr& dst, TexturePtr& temp)
{
    cmdBuffer->MarkerBegin("BLUR");

    cmdBuffer->RegisterUAVUsageTexture(dst);
    cmdBuffer->RegisterUAVUsageTexture(temp);

    struct DrawData
    {
        int width;
        int height;
        float widthInv;
        float heightInv;
        int blurTexture;
        int tempTexture;
        int isHorizontal;
    };

    DrawData drawData{};
    drawData.width = dst->GetWidth();
    drawData.height = dst->GetHeight();
    drawData.widthInv = 1.0f / dst->GetWidth();
    drawData.heightInv = 1.0f / dst->GetHeight();
    drawData.blurTexture = dst->BindUAV();
    drawData.tempTexture = temp->BindUAV();
    drawData.isHorizontal = 1;

    const PSOCompute* blurPso = PSOCompute::Get(Shader::GetCompute("assets/shaders/Blur.hlsl"));
    cmdBuffer->BindPsoCompute(blurPso);

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    cmdBuffer->Dispatch((drawData.width + 7) / 8, (drawData.height + 7) / 8, 1);
    
    cmdBuffer->RegisterUAVUsageTexture(temp);

    drawData.isHorizontal = 0;

    cmdBuffer->BindPsoCompute(blurPso);

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    cmdBuffer->Dispatch((drawData.width + 7) / 8, (drawData.height + 7) / 8, 1);

    cmdBuffer->MarkerEnd();
}

void HDRPostProcessRenderer::BloomUpsample(CommandBufferPtr& cmdBuffer, TexturePtr& src, TexturePtr& dst)
{
    cmdBuffer->RegisterSRVUsageTexture(src);
    cmdBuffer->RegisterUAVUsageTexture(dst);

    struct DrawData
    {
        int dstWidth;
        int dstHeight;
        float dstWidthInv;
        float dstHeightInv;
        float srcTexelWidth;
        float srcTexelHeight;
        int dstTexture;
        int srcTexture;
        int srcSampler;
    };

    DrawData drawData{};
    drawData.dstWidth = dst->GetWidth();
    drawData.dstHeight = dst->GetHeight();
    drawData.dstWidthInv = 1.0f / dst->GetWidth();
    drawData.dstHeightInv = 1.0f / dst->GetHeight();
    drawData.srcTexelWidth = 1.0f / src->GetWidth();
    drawData.srcTexelHeight = 1.0f / src->GetHeight();
    drawData.dstTexture = dst->BindUAV();
    drawData.srcTexture = src->BindSRV();
    drawData.srcSampler = Sampler::GetLinear().GetBindSlot();

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    const PSOCompute* bloomPso = PSOCompute::Get(Shader::GetCompute("assets/shaders/BloomUpsample.hlsl"));
    cmdBuffer->BindPsoCompute(bloomPso);

    cmdBuffer->Dispatch((drawData.dstWidth + 7) / 8, (drawData.dstHeight + 7) / 8, 1);
}