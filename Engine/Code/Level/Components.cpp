#include "Components.h"

#include <glm/gtx/quaternion.hpp>

void TransformComponent::UpdateTranslation(const glm::vec3& newTranslation)
{
    translation = newTranslation;

    RecalculateTransform();
}

void TransformComponent::UpdateScale(const glm::vec3& newScale)
{
    scale = newScale;

    RecalculateTransform();
}

void TransformComponent::UpdateRotation(const glm::vec3& newRotation)
{
    rotation = newRotation;

    RecalculateTransform();
}

void TransformComponent::RecalculateTransform()
{
    glm::mat4 rotationMat = glm::toMat4(glm::quat(glm::radians(rotation)));

    transform = glm::translate(glm::mat4(1.0f), translation) * rotationMat * glm::scale(glm::mat4(1.0f), scale);
}

void MeshComponent::Render(const glm::mat4& localTransform, const glm::mat4& parentTransform)
{
    glm::mat4 currentTransform = parentTransform * transforms.localTransform;

    Renderer* renderer = Renderer::Get();

    CommandBufferPtr cmdBuffer = renderer->GetLoadCmdBuffer();

    transforms.localTransform = localTransform;
    transforms.globalTransform = currentTransform;
    transforms.transposeInverseGlobalTransform = glm::transpose(glm::inverse(currentTransform));

    cmdBuffer->CopyToBuffer(transformsBuffer, &transforms, sizeof(transforms));

    for (RenderObjectPtr& renderObject : renderObjects)
    {
        renderer->SubmitRenderObject(renderObject);
    }
}