#include "Common.hlsli"

struct VertToPix
{
    float4 Position : SV_Position;
    float2 TC : TEXCOORD;
};

struct PixOut
{
    float4 Color : SV_Target0;
};

VertToPix MainVS(uint vtxId : SV_VertexID)
{
    VertToPix OUT = (VertToPix)0;

    if (vtxId == 0)
    {
        OUT.TC = float2(-1.0f, -1.0f);
        OUT.Position = float4(OUT.TC, 0.0f, 1.0f);
    }
    else if (vtxId == 1)
    {
        OUT.TC = float2(-1.0f, 3.0f);
        OUT.Position = float4(OUT.TC, 0.0f, 1.0f);
    }
    else if (vtxId == 2)
    {
        OUT.TC = float2(3.0f, -1.0f);
        OUT.Position = float4(OUT.TC, 0.0f, 1.0f);
    }

    return OUT;
}

struct DrawData
{
    Texture cubemapTexture;
    Sampler cubemapSampler;
    ArrayBuffer perFrameBuffer;
};

PUSH_CONSTANTS(DrawData, drawData);

PixOut MainPS(VertToPix IN)
{
    PixOut OUT = (PixOut)0;

    float4 clipSpace = float4(IN.TC, 1.0f, 1.0f);
    float3 direction = normalize(mul(drawData.perFrameBuffer.Load<PerFrameData>(0).invRotationViewProj, clipSpace).xyz);

    float3 envColor = SampleCubemap(drawData.cubemapTexture, drawData.cubemapSampler, direction).rgb;

    OUT.Color = float4(envColor, 1.0f);

    return OUT;
}