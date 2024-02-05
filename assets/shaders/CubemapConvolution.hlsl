#include "Common.hlsli"

struct DrawData
{
    Texture cubemapTexture;
    Sampler cubemapSampler;
    RWTexture convolutedCubemapFaces[6];
    float2 sizeInv;
};

PUSH_CONSTANTS(DrawData, drawData);

float3 RandomPCG3D(uint3 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return float3(v) * (1.0 / (float)0xffffffffu); 
}

groupshared float3 cumulativeColors[64];

[numthreads(64, 1, 1)]
void MainCS(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
    uint px = groupId.x;
    uint py = groupId.y;

    float3 normal = CalculateCubemapDirection(groupId.x, groupId.y, groupId.z, drawData.sizeInv, false);
    float3 up = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);

    float3 color = 0.0f;

#if 0 // Importance sampling. Produces worse results imho

    int sampleCount = 2 * 1024 * 1024;

    int samplesPerThread = sampleCount / 64;

    for (int i = 0; i < samplesPerThread; i++)
    {
        float3 random = RandomPCG3D(uint3(px, py, i * groupThreadId.x));

        float phi = TWO_PI * random.x;
        float theta = asin(sqrt(random.y));

        float3 tempVec = cos(phi) * right + sin(phi) * up;
        float3 worldDirection = cos(theta) * normal + sin(theta) * tempVec;

        color += cubemapTexture.SampleLevel(cubemapSampler, worldDirection, 0).rgb;
    }

    color /= (float)samplesPerThread;

#else

    cumulativeColors[groupThreadId.x] = 0.0f;

    float deltaPhi = TWO_PI / 2160.0f;
    float deltaTheta = HALF_PI / 540.0f;

    float phiThreadStep = 1.0f / 64.0f * TWO_PI;

    float phiStart = (float)groupThreadId.x * phiThreadStep;
    float phiEnd = phiStart + phiThreadStep;

    int sampleCount = 0;
    for (float phi = phiStart; phi < phiEnd; phi += deltaPhi)
    {
        for (float theta = 0.0f; theta < HALF_PI; theta += deltaTheta)
        {
            float3 tempVec = cos(phi) * right + sin(phi) * up;
            float3 worldDirection = cos(theta) * normal + sin(theta) * tempVec;

            color += cos(theta) * sin(theta) * drawData.cubemapTexture.SampleCubeLevel<float4>(drawData.cubemapSampler.Get(), worldDirection, 0).rgb;

            sampleCount++;
        }
    }

    color *= PI / (float)sampleCount;

#endif

    cumulativeColors[groupThreadId.x] = color;

    GroupMemoryBarrierWithGroupSync();

    if ((groupThreadId.x & 1) == 0)
    {
        cumulativeColors[groupThreadId.x] = cumulativeColors[groupThreadId.x] + cumulativeColors[groupThreadId.x + 1];
    }

    GroupMemoryBarrierWithGroupSync();

    if ((groupThreadId.x & 3) == 0)
    {
        cumulativeColors[groupThreadId.x] = cumulativeColors[groupThreadId.x] + cumulativeColors[groupThreadId.x + 2];
    }

    GroupMemoryBarrierWithGroupSync();

    if ((groupThreadId.x & 7) == 0)
    {
        cumulativeColors[groupThreadId.x] = cumulativeColors[groupThreadId.x] + cumulativeColors[groupThreadId.x + 4];
    }

    GroupMemoryBarrierWithGroupSync();

    if ((groupThreadId.x & 15) == 0)
    {
        cumulativeColors[groupThreadId.x] = cumulativeColors[groupThreadId.x] + cumulativeColors[groupThreadId.x + 8];
    }

    GroupMemoryBarrierWithGroupSync();

    if ((groupThreadId.x & 31) == 0)
    {
        cumulativeColors[groupThreadId.x] = cumulativeColors[groupThreadId.x] + cumulativeColors[groupThreadId.x + 16];
    }

    GroupMemoryBarrierWithGroupSync();

    if (groupThreadId.x == 0)
    {
        float3 colorResult = cumulativeColors[groupThreadId.x] + cumulativeColors[groupThreadId.x + 32];
        colorResult = colorResult / 64.0f;

        drawData.convolutedCubemapFaces[groupId.z].Store2D<float4>(groupId.xy, float4(colorResult, 1.0f));
    }
}