#include "Common.hlsli"

struct DrawData
{
    Texture cubemapTexture;
    Sampler cubemapSampler;
    RWTexture prefilteredCubemapFaces[6];
    float2 sizeInv;
    float2 cubemapSize;
    float roughness;
};

PUSH_CONSTANTS(DrawData, drawData);

[numthreads(8, 8, 1)]
void MainCS(uint3 DTid : SV_DispatchThreadID)
{
    float3 N = CalculateCubemapDirection(DTid.x, DTid.y, DTid.z, drawData.sizeInv, false);
    float3 R = N;
    float3 V = R;

    float roughness = drawData.roughness;

    float resolution = drawData.cubemapSize.x; // resolution of source cubemap (per face)
    float saTexel  = 4.0f * PI / (6.0f * resolution * resolution);

    float3 prefilteredColor = 0.0f;
    float totalWeight = 0.0f;

    const uint SAMPLE_COUNT = 1024;
    for(uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, roughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0f);
        if(NdotL > 0.0f)
        {
            float NdotH = dot(N, H);
            float HdotV = dot(H, V);
            float D   = DistributionGGX(NdotH, roughness);
            float pdf = (D * NdotH / (4.0f * HdotV)) + 0.0001f; 
            float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 

            prefilteredColor += drawData.cubemapTexture.SampleCubeLevel<float4>(drawData.cubemapSampler.Get(), L, mipLevel).rgb;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    drawData.prefilteredCubemapFaces[DTid.z].Store2D<float4>(DTid.xy, float4(prefilteredColor, 1.0f));
}