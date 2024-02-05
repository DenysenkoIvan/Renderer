#pragma once

#include "Material.h"
#include "Mesh.h"

#include "Backend/PSOGraphics.h"

enum class DrawCallType : uint64_t
{
    None = 0,
    ZPrePass = 1024,
    ZPrePassMesh = 2048,
    ZPass = 4096,
    ZPassMesh = 8192
};

struct RenderObject;
using RenderObjectPtr = std::shared_ptr<RenderObject>;

struct RenderObject
{
    BufferPtr perInstanceBuffer;
    MeshPtr mesh;
    MaterialPtr material;

    // for internal use
    std::unordered_map<DrawCallType, const PSOGraphics*> drawCallsPSOs;

    const PSOGraphics* GetPSO(DrawCallType type);

    static RenderObjectPtr Create();
};