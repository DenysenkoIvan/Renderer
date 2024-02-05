#include "Common.hlsli"

struct DrawData
{
    int bloomWidth;
    int bloomHeight;
    float bloomWidthInv;
    float bloomHeightInv;
    float hdrWidthInv;
    float hdrHeightInv;
    Texture hdrTexture;
    Sampler hdrSampler;
    RWTexture bloomTexture;
};

PUSH_CONSTANTS(DrawData, drawData);

float3 SampleHDRTexture(float2 uv, float2 texelSize, float threshold)
{
    if (uv.x <= 2.0f * drawData.hdrWidthInv || uv.y <= 2.0f * drawData.hdrHeightInv ||
        uv.x >= 1.0f - 2.0f * drawData.hdrWidthInv || uv.y >= 1.0f - 2.0f * drawData.hdrHeightInv)
    {
        return 0.0f;
    }
    
    float4 value = drawData.hdrTexture.Sample2DLevel<float4>(drawData.hdrSampler.Get(), uv, 0.0f);

    value.rgb = max(0.0f, value.rgb - float3(threshold, threshold, threshold));

    return value.rgb;
}

float3 SampleBloomTexture(uint2 pixel)
{
    pixel = clamp(pixel, 0, uint2(drawData.bloomWidth - 1, drawData.bloomHeight - 1));

    return drawData.bloomTexture.Load2D<float4>(pixel).rgb;
}

[numthreads(8, 8, 1)]
void MainCS(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixel = DTid.xy;
    if (pixel.x >= drawData.bloomWidth || pixel.y >= drawData.bloomHeight)
    {
        return;
    }

    float2 uv = (float2(pixel) + float2(0.5f, 0.5f)) * float2(drawData.bloomWidthInv, drawData.bloomHeightInv);

    float2 texelSize = float2(drawData.hdrWidthInv, drawData.hdrHeightInv);

    // A . B . C
    // . D . E .
    // F . G . H
    // . I . J .
    // K . L . M

    float3 A = SampleHDRTexture(uv + float2(-2.0f, -2.0f) * texelSize, texelSize, 1.0f);
    float3 B = SampleHDRTexture(uv + float2( 0.0f, -2.0f) * texelSize, texelSize, 1.0f);
    float3 C = SampleHDRTexture(uv + float2( 2.0f, -2.0f) * texelSize, texelSize, 1.0f);
    float3 D = SampleHDRTexture(uv + float2(-1.0f, -1.0f) * texelSize, texelSize, 1.0f);
    float3 E = SampleHDRTexture(uv + float2( 1.0f, -1.0f) * texelSize, texelSize, 1.0f);
    float3 F = SampleHDRTexture(uv + float2(-2.0f,  0.0f) * texelSize, texelSize, 1.0f);
    float3 G = SampleHDRTexture(uv + float2( 0.0f,  0.0f) * texelSize, texelSize, 1.0f);
    float3 H = SampleHDRTexture(uv + float2( 2.0f,  0.0f) * texelSize, texelSize, 1.0f);
    float3 I = SampleHDRTexture(uv + float2(-1.0f,  1.0f) * texelSize, texelSize, 1.0f);
    float3 J = SampleHDRTexture(uv + float2( 1.0f,  1.0f) * texelSize, texelSize, 1.0f);
    float3 K = SampleHDRTexture(uv + float2(-2.0f,  2.0f) * texelSize, texelSize, 1.0f);
    float3 L = SampleHDRTexture(uv + float2( 0.0f,  2.0f) * texelSize, texelSize, 1.0f);
    float3 M = SampleHDRTexture(uv + float2( 2.0f,  2.0f) * texelSize, texelSize, 1.0f);

    float3 o = 0.0f;
    o += (D.rgb + E.rgb + I.rgb + J.rgb) * 0.500f * 0.25f;
    o += (A.rgb + B.rgb + F.rgb + G.rgb) * 0.125h * 0.25h;
    o += (B.rgb + C.rgb + G.rgb + H.rgb) * 0.125h * 0.25h;
    o += (F.rgb + G.rgb + K.rgb + L.rgb) * 0.125h * 0.25h;
    o += (G.rgb + H.rgb + L.rgb + M.rgb) * 0.125h * 0.25h;

    drawData.bloomTexture.Store2D<float4>(pixel, float4(o, 1.0f));
}