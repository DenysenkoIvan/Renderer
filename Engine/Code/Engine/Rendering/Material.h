#pragma once

#include "Backend/Buffer.h"
#include "Backend/Texture.h"

#include <glm/glm.hpp>

#include <string>

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
    glm::vec4 albedo = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 aoMetRough = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    glm::vec4 emissiveValue = glm::vec4(0.0f);
    glm::vec4 specular = glm::vec4(1.0f); // x, y, z - color, a - strength
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.0f;
    int isDoubleSided = 0;
    float normalScale = 1.0f;
    Workflow workflow = Workflow::MetallicRoughness;

    int albedoMap;
    int normalsMap;
    int aoRoughMetMap;
    int emissiveMap;
    int specularMap;
    int occlusionMap;
    int sampler;
};

struct Material;
using MaterialPtr = std::shared_ptr<Material>;

struct Material
{
    std::string name;
    MaterialProps props{};
    BufferPtr propsBuffer;
    TexturePtr albedoTexture;
    TexturePtr normalsTexture;
    TexturePtr metRoughTexture;
    TexturePtr emissiveTexture;
    TexturePtr specularTexture;
    TexturePtr occlusionTexture;

    static MaterialPtr Create()
    {
        return std::make_shared<Material>();
    }
};