#ifndef COMMON_H
#define COMMON_H

#include "Base.hlsli"

static const float PI = 3.14159265358979323846f;
static const float TWO_PI = 2.0f * PI;
static const float HALF_PI = 0.5f * PI;
static const float POS_INFINITY = 1.#INF;
static const float NEG_INFINITY = -1.#INF;

struct PerFrameData
{
    float4x4 projMatrix;
    float4x4 viewMatrix;
    float4x4 projView;
    float4x4 invRotationViewProj;
    float4 cameraPosition;
    float4 lightDirection;
    float4 lightColorIntensity;
    Texture brdfLutTexture;
    Texture convolutedCubemapTexture;
    Texture prefilteredCubemapTextures[5];
    Sampler samplerDesc;
};

float2 ToDixectXCoordSystem(float2 pos)
{
    float2 OUT;

    OUT.x = (pos.x * 2.0h) - 1.0h;
    OUT.y = (1.0h - pos.y) * 2.0h - 1.0h;
    
    return OUT;
}

float2 FromDirectXCoordSystem(float2 pos)
{
    float2 OUT;

    OUT.x = (pos.x + 1.0f) / 2.0f;
    OUT.y = 1.0f - (pos.y + 1.0f) / 2.0f;

    return OUT;
}

float4x4 RemoveTranslation(float4x4 IN)
{
    float4x4 OUT = IN;

    OUT[0][3] = 0.0f;
    OUT[1][3] = 0.0f;
    OUT[2][3] = 0.0f;

    return OUT;
}

enum class AlphaMode : int
{
    Opaque = 0,
    Blend = 1,
    Mask = 2
};

enum class Workflow : int
{
    MetallicRoughness = 1,
    SpecularGlossiness = 2
};

struct MaterialProps
{
    float4 albedo;
    float4 aoMetRough;
    float4 emissive;
    float4 specular;
    int alphaMode;
    float alphaCutoff;
    int isDoubleSided;
    float normalScale;
    int workflow;

    Texture albedoMap;
    Texture normalsMap;
    Texture aoRoughMetMap;
    Texture emissiveMap;
    Texture specularMap;
    Texture occlusionMap;
    Sampler samplerDesc;
};

float4 SampleCubemap(Texture texture, Sampler textureSampler, float3 direction)
{
    direction.z *= -1.0f;
    direction.y *= -1.0f;
    direction = direction.xzy;
    direction = normalize(direction);

    return texture.SampleCube<float4>(textureSampler.Get(), direction);
}

float4 SamplePrefilteredCubemap(Texture cubemaps[5], Sampler samplerDesc, float3 direction, int mip)
{
    if (mip == 0)
    {
        return SampleCubemap(cubemaps[0], samplerDesc, direction);
    }
    else if (mip == 1)
    {
        return SampleCubemap(cubemaps[1], samplerDesc, direction);
    }
    else if (mip == 2)
    {
        return SampleCubemap(cubemaps[2], samplerDesc, direction);
    }
    else if (mip == 3)
    {
        return SampleCubemap(cubemaps[3], samplerDesc, direction);
    }
    else if (mip == 4)
    {
        return SampleCubemap(cubemaps[4], samplerDesc, direction);
    }

    return 0.0f;
}

float4 SamplePrefilteredCubemapLinear(Texture cubemaps[5], Sampler samplerDesc, float3 direction, float mip)
{
    float fractional = frac(mip);

    int mip0 = mip;
    float4 value0 = SamplePrefilteredCubemap(cubemaps, samplerDesc, direction, mip0);
    float4 value1 = value0;

    int mip1 = mip + 1;
    if (fractional > 0.01f && mip1 <= 6)
    {
        value1 = SamplePrefilteredCubemap(cubemaps, samplerDesc, direction, mip1);
    }

    return lerp(value0, value1, fractional);
}

float3 CalculateCubemapDirection(float px, float py, uint face, float2 faceSizesInv, bool isZAxisUp = true)
{
    float2 uvc = (float2(px, py) + 0.5f) * faceSizesInv;
    uvc = 2.0f * uvc - 1.0f;
    uvc.y *= -1.0f;

    float3 dir;
    if (face == 0)
    {
        dir = float3(1.0f, uvc.y, -uvc.x);
    }
    else if (face == 1)
    {
        dir = float3(-1.0f, uvc.y, uvc.x);
    }
    if (face == 2)
    {
        dir = float3(uvc.x, 1.0f, -uvc.y);
    }
    if (face == 3)
    {
        dir = float3(uvc.x, -1.0f, uvc.y);
    }
    if (face == 4)
    {
        dir = float3(uvc.x, uvc.y, 1.0f);
    }
    if (face == 5)
    {
        dir = float3(-uvc.x, uvc.y, -1.0f);
    }

    if (isZAxisUp)
    {
        dir = dir.xzy;
    }

    return normalize(dir);
}

float2 SampleSphericalMap(float3 v)
{
    const float2 invAtan = float2(1.0f / TWO_PI, 1.0f / PI);

    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5f;

    return uv;
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // from tangent-space vector to world-space sample vector
    float3 up        = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;

    return normalize(sampleVec);
}

float DistributionGGX(float NdotH, float roughness) {
    NdotH = saturate(NdotH);
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;
    return a2 / max(denom, 0.000001f);
}

float min_float3(float3 input)
{
    return min(input.x, min(input.y, input.z));
}

float max_float3(float3 input)
{
    return max(input.x, max(input.y, input.z));
}

#endif // COMMON_H