#pragma once

#include <imgui.h>
#include <ImGuizmo.h>

#include <Framework/Common.h>
#include <Engine/Engine.h>
#include <Level/Entity.h>

class LevelInspector
{
public:
    void SetFonts(ImFont* regularFont, ImFont* titleFont);

    void Update();

    const std::string& GetName() const;

private:
    void DisplayEntity(Entity entity);

private:
    std::string m_name = "Level inspector";

    ImFont* m_regularFont = nullptr;
    ImFont* m_titleFont = nullptr;

    ImGuizmo::OPERATION m_imguizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
    bool m_isSnap = false;
    Entity m_selectedEntity;
};