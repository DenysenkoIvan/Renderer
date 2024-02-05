#include "InfoPanel.h"

#include <Loaders/MeshLoader.h>

void InfoPanel::SetFonts(ImFont* regularFont, ImFont* titleFont)
{
    Assert(regularFont && titleFont);

    m_regularFont = regularFont;
    m_titleFont = titleFont;
}

void InfoPanel::Update()
{
    ImGui::PushFont(m_titleFont);
    ImGui::Begin(m_name.c_str());

    ImGui::PushFont(m_regularFont);

    Engine* engine = Engine::Get();
    const EngineStats& engineStats = engine->GetStats();
    const Camera& camera = engine->GetCamera();
    glm::vec3 cameraPos = camera.GetPosition();
    glm::vec3 cameraView = camera.GetFront();

    ImGui::Text("FPS: %.1f", engineStats.fps);
    ImGui::Text("Frametime: %.1fms", engineStats.frametime);
    ImGui::Text("Camera Position  (%.1f, %.1f, %.1f)", cameraPos.x, cameraPos.y, cameraPos.z);
    ImGui::Text("Camera Direction (%.1f, %.1f, %.1f)", cameraView.x, cameraView.y, cameraView.z);

    ImGui::Separator();

    static const char* windowModes[] = { "Windowed", "Windowed (Borderless)" };
    static const char* currentItem = windowModes[0];
    bool currentItemChanged = false;
    if (ImGui::BeginCombo("Window", currentItem))
    {
        for (int i = 0; i < 2; i++)
        {
            bool isSelected = currentItem == windowModes[i];
            if (ImGui::Selectable(windowModes[i], isSelected))
            {
                currentItemChanged = currentItem != windowModes[i];
                currentItem = windowModes[i];
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();

        if (currentItemChanged)
        {
            if (currentItem == windowModes[0])
            {
                Window::Get()->ExitFullscreenBorderless();
            }
            else if (currentItem == windowModes[1])
            {
                Window::Get()->EnterFullscreenBorderless();
            }
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Load mesh"))
    {
        std::filesystem::path meshPath = Window::Get()->OpenFileDialog();
        if (!meshPath.empty())
        {
            MeshLoader::Load(meshPath);
        }
    }
    if (ImGui::Button("Load skybox"))
    {
        std::filesystem::path skyboxPath = Window::Get()->OpenFileDialog();

        if (!skyboxPath.empty())
        {
            auto ext = skyboxPath.extension().generic_string();
            if (skyboxPath.extension() == ".hdr")
            {
                engine->LoadSkybox(skyboxPath);
            }
            else
            {
                LogError("File has extension {}, but expected .hdr", skyboxPath.extension().generic_string());
            }
        }
    }

    ImGui::Separator();

    static bool value = true;
    ImGui::Checkbox("ZPrepass", &engine->GetRenderer()->GetProps().isUseZPrepass);

    ImGui::PopFont();
    ImGui::PopFont();
    ImGui::End();
}

const std::string& InfoPanel::GetName() const
{
    return m_name;
}