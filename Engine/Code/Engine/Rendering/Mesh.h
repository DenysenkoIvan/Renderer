#pragma once

#include "Material.h"

#include "Backend/Buffer.h"

enum VertexComponentFlagBits : uint32_t
{
    VertexComponentNone = 0,
    VertexComponentTangentBitangents = 1,
    VertexComponentUvs = 2,
    VertexComponentColors = 4
};
using VertexComponentFlags = uint32_t;

struct Meshlet
{
    float center[3]{};
    float radius = 0.0f;
    float coneApex[3]{};
    float coneAxis[3]{};
    float cutoff = 0.0f;
    uint32_t vertexOffset = 0;
    uint32_t vertexCount = 0;
    uint32_t triangleOffset = 0;
    uint32_t triangleCount = 0;
};

struct Mesh;
using MeshPtr = std::shared_ptr<Mesh>;

struct Mesh
{
    BufferPtr positions;
    BufferPtr indexBuffer;
    BufferPtr meshlets;
    BufferPtr vertices;
    BufferPtr meshletVertices;
    BufferPtr meshletTriangles;
    int meshletsCount = 0;
    VertexComponentFlags components = VertexComponentNone;

    static MeshPtr Create()
    {
        return std::make_shared<Mesh>();
    }
};