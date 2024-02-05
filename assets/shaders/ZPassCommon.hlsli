#ifndef Z_PASS_COMMON
#define Z_PASS_COMMON

#include "Common.hlsli"

struct ModelMatrix
{
    float4x4 localTransform;
    float4x4 globalTransform;
    float4x4 transpInvGlobalTransform;
};

struct Vertex
{
    float3 Position;
    half NormalX, NormalY, NormalZ;

    #if defined(USE_TANGENTS_BITANGENTS)
        half TangentX, TangentY, TangentZ;
        half BitangentX, BitangentY, BitangentZ;
    #endif

    #if defined(USE_UV)
        half TC_U, TC_V;
    #endif

    #if defined(USE_VERTEX_COLOR)
        half ColorR, ColorG, ColorB, ColorA;
    #endif
};

struct DrawData
{
    int isBackFaceCull;
    ArrayBuffer indices;
    ArrayBuffer vertices;
    ArrayBuffer meshlets;
    uint meshletCount;
    ArrayBuffer meshletIndices;
    ArrayBuffer meshletVertices;
    ArrayBuffer materialProps;
    ArrayBuffer perFrameBuffer;
    ArrayBuffer perInstanceBuffer;
};

PUSH_CONSTANTS(DrawData, drawData);

#if defined(USE_MESH_SHADING)

#define THREADS_PER_GROUP 32
#define MAX_VERTICES_COUNT 64
#define MAX_TRIANGLES_COUNT 84

struct Meshlet
{
    float3 center;
    float radius;
    float3 coneApex;
    float3 coneAxis;
    float coneCutoff;
    uint vertexOffset;
    uint vertexCount;
    uint triangleOffset;
    uint triangleCount;
};

struct Payload
{
    uint meshletIndices[32];
};

groupshared Payload payload;

bool ConeCull(float3 coneApex, float3 coneAxis, float coneCutoff, float3 cameraPosition)
{
    return dot(normalize(coneApex - cameraPosition), coneAxis) >= coneCutoff;
}

[numthreads(THREADS_PER_GROUP, 1, 1)]
void MainTS(in uint3 groupThreadId : SV_GroupThreadID,
            in uint3 groupId : SV_GroupID,
            in uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint meshletId = dispatchThreadId.x;

    if (meshletId >= drawData.meshletCount)
    {
        return;
    }

    bool accept = true;
    if (drawData.isBackFaceCull)
    {
        Meshlet meshlet = drawData.meshlets.Load<Meshlet>(meshletId);
        const float4x4 globalTransform = drawData.perInstanceBuffer.Load<ModelMatrix>().globalTransform;

        float4 coneApex = mul(globalTransform, float4(meshlet.coneApex, 1.0f));
        coneApex.xyz /= coneApex.w;

        float3 coneAxis = normalize(mul(globalTransform, float4(meshlet.coneAxis, 0.0f)).xyz);
        float coneCutoff = meshlet.coneCutoff;
        float3 cameraPosition = drawData.perFrameBuffer.Load<PerFrameData>().cameraPosition.xyz;

        bool accept = !ConeCull(coneApex.xyz, coneAxis, coneCutoff, cameraPosition);
    }

    uint arrayIndex = WavePrefixCountBits(accept);
    uint result = WaveActiveCountBits(accept);

    if (accept)
    {
        payload.meshletIndices[arrayIndex] = meshletId;
    }

    if (groupThreadId.x == 0)
    {
        DispatchMesh(result, 1, 1, payload);
    }
}

#endif // USE_MESH_SHADING

#endif