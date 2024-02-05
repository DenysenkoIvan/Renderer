#include "Common.hlsli"

struct DrawData
{
    Texture equirectangularMap;
    Sampler equirectangularSampler;
    RWTexture cubemapFaces[6];
    float2 sizeInv;
};

PUSH_CONSTANTS(DrawData, drawData);

[numthreads(64, 1, 1)]
void MainCS(uint3 DTid : SV_DispatchThreadID)
{
    float3 direction = CalculateCubemapDirection(DTid.x, DTid.y, DTid.z, drawData.sizeInv, false);
    float2 equirectangularUV = SampleSphericalMap(direction);

    float3 color = drawData.equirectangularMap.Sample2DLevel<float4>(drawData.equirectangularSampler.Get(), equirectangularUV, 0).rgb;

    drawData.cubemapFaces[DTid.z].Store2D<float4>(DTid.xy, float4(color, 1.0f));
}