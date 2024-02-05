#pragma once

#include <array>

#include "Buffer.h"
#include "Texture.h"

struct CommonRenderResources
{
    BufferPtr perFrameBuffer;
    TexturePtr hdrTarget;
    TexturePtr depthTarget;
    std::vector<TexturePtr> bloomTextures;
    std::vector<TexturePtr> hdrTargetHalfs;
    TexturePtr skybox;
    TexturePtr convolutedSkybox;
    TexturePtr brdfLut;
    std::array<TexturePtr, 5> prefilteredCubemap;
};