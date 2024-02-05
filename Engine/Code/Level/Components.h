#pragma once

#include <string>

#include <glm/glm.hpp>

#include <Engine/Rendering/Renderer.h>

#include "Entity.h"

struct TagComponent
{
    std::string tag;
};

struct TransformComponent
{
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::mat4 transform = glm::mat4(1.0f);

    void UpdateTranslation(const glm::vec3& newTranslation);
    void UpdateScale(const glm::vec3& newScale);
    void UpdateRotation(const glm::vec3& newRotation);

private:
    void RecalculateTransform();
};

struct MeshComponent
{
    struct Transforms
    {
        glm::mat4 localTransform = glm::mat4(1.0f);
        glm::mat4 globalTransform = glm::mat4(1.0f);
        glm::mat4 transposeInverseGlobalTransform = glm::mat3(1.0f);
    };

    std::string name;
    Transforms transforms{};
    BufferPtr transformsBuffer;
    std::vector<RenderObjectPtr> renderObjects;

    void Render(const glm::mat4& localTransform, const glm::mat4& parentTransform);
};