#include "Common.hlsli"

struct DrawData
{
    int dstWidth;
    int dstHeight;
    float dstWidthInv;
    float dstHeightInv;
    float srcTexelWidth;
    float srcTexelHeight;
    RWTexture dstTexture;
    Texture srcTexture;
    Sampler srcSampler;
};

PUSH_CONSTANTS(DrawData, drawData);

float4 Upsample1Fetch(float2 uv)
{
    return drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv, 0.0f);
}

float4 Upsample9Fetches(float2 uv)
{
    float4 o = 0.0f;
    
    o += 4.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv, 0.0f);
    
    o += 1.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv + float2(-drawData.srcTexelWidth, -drawData.srcTexelHeight), 0.0f);
    o += 2.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv + float2(0.0f, -drawData.srcTexelHeight), 0.0f);
    o += 1.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv + float2(drawData.srcTexelWidth, -drawData.srcTexelHeight), 0.0f);
    
    o += 2.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv + float2(-drawData.srcTexelWidth, 0.0f), 0.0f);
    o += 2.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv + float2(drawData.srcTexelWidth, 0.0f), 0.0f);
    
    o += 1.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv + float2(-drawData.srcTexelWidth, drawData.srcTexelHeight), 0.0f);
    o += 2.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv + float2(0.0f, drawData.srcTexelHeight), 0.0f);
    o += 1.0f * drawData.srcTexture.Sample2DLevel<float4>(drawData.srcSampler.Get(), uv + float2(drawData.srcTexelWidth, drawData.srcTexelHeight), 0.0f);
    
    return o /= 16.0f;
}

[numthreads(8, 8, 1)]
void MainCS(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixel = DTid.xy;
    if (pixel.x >= drawData.dstWidth || pixel.y >= drawData.dstHeight)
    {
        return;
    }

    float2 uv = (float2(pixel) + float2(0.5f, 0.5f)) * float2(drawData.dstWidthInv, drawData.dstHeightInv);

    float4 dstColor = drawData.dstTexture.Load2D<float4>(pixel);

#if 1
    float4 srcColor = Upsample9Fetches(uv);
#else
    float4 srcColor = Upsample1Fetch(uv);
#endif

    float3 o = dstColor.rgb + srcColor.rgb;

    drawData.dstTexture.Store2D<float4>(pixel, float4(o, 1.0f));
}