#include "LevelInspector.h"

#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo.h>

#include "Application/Application.h"

void LevelInspector::SetFonts(ImFont* regularFont, ImFont* titleFont)
{
    m_regularFont = regularFont;
    m_titleFont = titleFont;
}

void LevelInspector::Update()
{
    ImGui::PushFont(m_titleFont);
    ImGui::Begin(m_name.c_str());
    ImGui::PopFont();

    Level* level = Application::Get()->GetLevel();
    if (!level)
    {
        ImGui::PushFont(m_regularFont);
        ImGui::Text("empty");
        ImGui::PopFont();
        ImGui::End();
        return;
    }

    if (ImGui::IsWindowFocused() && Input::IsKeyReleased(Key::Esc))
    {
        m_selectedEntity = Entity();
    }

    ImGui::PushFont(m_regularFont);

    for (Entity entity : level->GetRootEntities())
    {
        DisplayEntity(entity);
    }

    ImGui::Separator();

    if (m_selectedEntity.IsValid())
    {
        TransformComponent& transformComponent = m_selectedEntity.GetComponent<TransformComponent>();
        glm::vec3 translation = transformComponent.translation;
        if (ImGui::DragFloat3("Translation", glm::value_ptr(translation), 0.01f, 0.0f, 0.0f, "%.2f"))
        {
            transformComponent.UpdateTranslation(translation);
        }
        glm::vec3 scale = transformComponent.scale;
        if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f, 0.0f, 0.0f, "%.2f"))
        {
            transformComponent.UpdateScale(scale);
        }
        glm::vec3 rotation = transformComponent.rotation;
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 0.01f, 0.0f, 0.0f, "%.2f"))
        {
            transformComponent.UpdateRotation(rotation);
        }

        ImGuizmo::SetOrthographic(true);
        ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());

        glm::uvec2 size = Window::Get()->GetClientAreaSize();
        ImGuizmo::SetRect(0.0f, 0.0f, (float)size.x, (float)size.y);

        const Camera& camera = Engine::Get()->GetCamera();
        glm::mat4 view = camera.ViewMatrix();
        glm::mat4 proj = camera.ProjMatrix();

        glm::mat4 allParentsTransform(1.0f);
        Entity parent = m_selectedEntity.GetParent();
        while (parent.IsValid())
        {
            TransformComponent& parentTransform = parent.GetComponent<TransformComponent>();
            allParentsTransform = parentTransform.transform * allParentsTransform;
            parent = parent.GetParent();
        }

        glm::mat4 worldTransform = allParentsTransform * transformComponent.transform;

        if (ImGui::RadioButton("Translation", m_imguizmoOperation == ImGuizmo::OPERATION::TRANSLATE))
        {
            m_imguizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotation", m_imguizmoOperation == ImGuizmo::OPERATION::ROTATE))
        {
            m_imguizmoOperation = ImGuizmo::OPERATION::ROTATE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", m_imguizmoOperation == ImGuizmo::OPERATION::SCALE))
        {
            m_imguizmoOperation = ImGuizmo::OPERATION::SCALE;
        }

        ImGui::SameLine();
        ImGui::Checkbox("Snap", &m_isSnap);

        glm::vec3 snapValue(0.5f);
        if (m_imguizmoOperation == ImGuizmo::OPERATION::TRANSLATE)
        {
            snapValue = glm::vec3(0.5f);
        }
        else if (m_imguizmoOperation == ImGuizmo::OPERATION::ROTATE)
        {
            snapValue = glm::vec3(45.0f);
        }
        else if (m_imguizmoOperation == ImGuizmo::OPERATION::SCALE)
        {
            snapValue = glm::vec3(0.25f);
        }

        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), m_imguizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(worldTransform), nullptr, m_isSnap ? glm::value_ptr(snapValue) : nullptr);
        glm::mat4 newLocalTransform = glm::inverse(allParentsTransform) * worldTransform;

        if (ImGuizmo::IsUsing())
        {
            glm::vec3 newTranslaion, newRotation, newScale;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(newLocalTransform), glm::value_ptr(newTranslaion), glm::value_ptr(newRotation), glm::value_ptr(newScale));
            if (m_imguizmoOperation == ImGuizmo::OPERATION::TRANSLATE)
            {
                transformComponent.UpdateTranslation(newTranslaion);
            }
            else if (m_imguizmoOperation == ImGuizmo::OPERATION::ROTATE)
            {
                transformComponent.UpdateRotation(newRotation);
            }
            else if (m_imguizmoOperation == ImGuizmo::OPERATION::SCALE)
            {
                transformComponent.UpdateScale(newScale);
            }
        }
    }

    ImGui::PopFont();
    ImGui::End();
}

const std::string& LevelInspector::GetName() const
{
    return m_name;
}

void LevelInspector::DisplayEntity(Entity entity)
{
    if (!entity.IsValid())
    {
        return;
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (!entity.HasChildren())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (entity == m_selectedEntity)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    bool isOpened = ImGui::TreeNodeEx((void*)entity.GetId(), flags, entity.GetName().data());
    if (ImGui::IsItemClicked())
    {
        m_selectedEntity = entity;
    }

    if (isOpened)
    {
        for (Entity child : entity.GetChildren())
        {
            DisplayEntity(child);
        }
        ImGui::TreePop();
    }
}