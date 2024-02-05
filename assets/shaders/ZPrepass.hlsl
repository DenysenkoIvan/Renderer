#include "Common.hlsli"
#include "ZPassCommon.hlsli"

struct VertToPix
{
    float4 Position : SV_Position;
};

#if defined(USE_MESH_SHADING)

[numthreads(THREADS_PER_GROUP, 1, 1)]
[OutputTopology("triangle")]
void MainMS(in uint3 groupId : SV_GroupID,
            in uint3 groupThreadId : SV_GroupThreadID,
            in payload Payload pl,
            out vertices VertToPix vertices[MAX_VERTICES_COUNT],
            out indices uint3 triangles[MAX_TRIANGLES_COUNT])
{
    const uint meshletId = pl.meshletIndices[groupId.x];

    if (meshletId >= MeshInfo.meshletCount)
    {
        return;
    }

    const uint vtxCount = Meshlets[meshletId].vertexCount;
    const uint triangleCount = Meshlets[meshletId].triangleCount;

    if (groupThreadId.x == 0)
    {
        SetMeshOutputCounts(vtxCount, triangleCount);
    }

    const float4x4 globalTransform = Model.globalTransform;
    const float4x4 projView = PerFrame.projView;
    const float3x3 transpInvGlobalTransform = Model.transpInvGlobalTransform;

    const uint vertexOffset = Meshlets[meshletId].vertexOffset;
    for (uint i = groupThreadId.x; i < vtxCount; i += THREADS_PER_GROUP)
    {
        const uint vertexIndex = MeshletVertices[vertexOffset + i];

        VertToPix OUT = (VertToPix)0;

        float4 worldPosition = mul(globalTransform, float4(Positions[vertexIndex], 1.0f));
        OUT.Position = mul(projView, worldPosition);
        OUT.Position.y *= -1.0f;

        vertices[i] = OUT;
    }

    const uint triangleOffset = Meshlets[meshletId].triangleOffset;
    for (uint i = groupThreadId.x; i < triangleCount; i += THREADS_PER_GROUP)
    {
        uint indices = MeshletIndices[triangleOffset + i];
        triangles[i] = uint3(indices & 0xff, (indices >> 8) & 0xff, (indices >> 16) & 0xff);
    }
}

#else // USE_MESH_SHADING

struct DrawData
{
    ArrayBuffer positions;
    ArrayBuffer indices;
    ArrayBuffer perFrameBuffer;
    ArrayBuffer perInstanceBuffer;
};

PUSH_CONSTANTS(DrawData, drawData);

VertToPix MainVS(uint vertexId : SV_VertexID)
{
    VertToPix OUT = (VertToPix)0;

    uint vertexIndex = vertexId;
    if (drawData.indices.IsValid())
    {
        vertexIndex = drawData.indices.Load<uint>(vertexId);
    }

    float4 worldPosition = mul(drawData.perInstanceBuffer.Load<ModelMatrix>(0).globalTransform, float4(drawData.positions.Load<float3>(vertexIndex), 1.0f));
    OUT.Position = mul(drawData.perFrameBuffer.Load<PerFrameData>(0).projView, worldPosition);

    return OUT;
}

#endif // USE_MESH_SHADING