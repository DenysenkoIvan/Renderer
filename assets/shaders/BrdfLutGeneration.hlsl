#include "Base.hlsli"
#include "Common.hlsli"

struct DrawData
{
    RWTexture brdfLut;
    float2 sizeInv;
};

PUSH_CONSTANTS(DrawData, drawData);

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0f;

    float nom   = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float2 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V = float3(sqrt(1.0 - NdotV * NdotV), 0.0f, NdotV);

    float A = 0.0f;
    float B = 0.0f;

    float3 N = float3(0.0f, 0.0f, 1.0f);

    const uint SAMPLE_COUNT = 2048;
    for(uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, roughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V, H), 0.0f);

        if(NdotL > 0.0f)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    return float2(A, B);
}

[numthreads(64, 1, 1)]
void MainCS(uint3 DTid : SV_DispatchThreadID)
{
    float2 uv = ((float2)DTid.xy + 0.5f) * drawData.sizeInv;

    drawData.brdfLut.Store2D<float2>(DTid.xy, IntegrateBRDF(uv.x, 1.0f - uv.y));
}