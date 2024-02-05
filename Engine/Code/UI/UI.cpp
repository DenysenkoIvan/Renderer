#include "UI.h"

#include <ImGuizmo.h>
#include <imgui_internal.h>

#include <Framework/Common.h>

#include <Input/Input.h>
#include <Engine/Engine.h>
#include <Window/Window.h>

UI* UI::s_ui = nullptr;

UI::UI()
{
    ProfileFunction();

    ImGui::CreateContext();

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().IniFilename = nullptr;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));

    LoadFonts();
    RemapKeys();

    m_infoPanel.SetFonts(m_regularFont, m_titleFont);
    m_levelInspector.SetFonts(m_regularFont, m_titleFont);
    m_statsPanel.SetFonts(m_regularFont, m_titleFont);
}

void UI::OnTick(float deltaTime)
{
    ProfileFunction();

    if (Window::Get()->IsMinimized())
    {
        ResetInputs();
        return;
    }

    UpdateIO(deltaTime);

    if (!ImGui::GetIO().Fonts->IsBuilt())
    {
        return;
    }

    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    if (Input::IsKeyReleased(Key::Tilde))
    {
        m_isShowUi = !m_isShowUi;
    }

    if (!m_isShowUi)
    {
        return;
    }

    BeginDock();

    static bool isFirstTime = true;
    if (isFirstTime)
    {
        SetupLayout();
        isFirstTime = false;
    }

    m_infoPanel.Update();
    m_levelInspector.Update();
    m_statsPanel.Update();

    EndDock();
}

UI* UI::Get()
{
    return s_ui;
}

UIPtr UI::Create()
{
    Assert(s_ui == nullptr);

    UIPtr ui = std::make_unique<UI>();

    s_ui = ui.get();

    return ui;
}

void UI::LoadFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontAtlas* fonts = io.Fonts;

    ImFontConfig config{};
    config.MergeMode = false;
    m_regularFont = fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 14.0f, &config);

    config.MergeMode = true;
    m_titleFont = fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consolab.ttf", 16.0f, &config);
    fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 14.0f, &config, fonts->GetGlyphRangesCyrillic());
    fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consolab.ttf", 16.0f, &config, fonts->GetGlyphRangesCyrillic());

    io.FontDefault = m_regularFont;
}

void UI::RemapKeys()
{
    ImGuiIO& io = ImGui::GetIO();

#define KEY_TO_INT(key) (int)key

    io.KeyMap[ImGuiKey_Escape] = KEY_TO_INT(Key::Esc);
    io.KeyMap[ImGuiKey_F1] = KEY_TO_INT(Key::F1);
    io.KeyMap[ImGuiKey_F2] = KEY_TO_INT(Key::F2);
    io.KeyMap[ImGuiKey_F3] = KEY_TO_INT(Key::F3);
    io.KeyMap[ImGuiKey_F4] = KEY_TO_INT(Key::F4);
    io.KeyMap[ImGuiKey_F5] = KEY_TO_INT(Key::F5);
    io.KeyMap[ImGuiKey_F6] = KEY_TO_INT(Key::F6);
    io.KeyMap[ImGuiKey_F7] = KEY_TO_INT(Key::F7);
    io.KeyMap[ImGuiKey_F8] = KEY_TO_INT(Key::F8);
    io.KeyMap[ImGuiKey_F9] = KEY_TO_INT(Key::F9);
    io.KeyMap[ImGuiKey_F10] = KEY_TO_INT(Key::F10);
    io.KeyMap[ImGuiKey_F11] = KEY_TO_INT(Key::F11);
    io.KeyMap[ImGuiKey_F12] = KEY_TO_INT(Key::F12);
    io.KeyMap[ImGuiKey_GraveAccent] = KEY_TO_INT(Key::Tilde);
    io.KeyMap[ImGuiKey_1] = KEY_TO_INT(Key::Num1);
    io.KeyMap[ImGuiKey_2] = KEY_TO_INT(Key::Num2);
    io.KeyMap[ImGuiKey_3] = KEY_TO_INT(Key::Num3);
    io.KeyMap[ImGuiKey_4] = KEY_TO_INT(Key::Num4);
    io.KeyMap[ImGuiKey_5] = KEY_TO_INT(Key::Num5);
    io.KeyMap[ImGuiKey_6] = KEY_TO_INT(Key::Num6);
    io.KeyMap[ImGuiKey_7] = KEY_TO_INT(Key::Num7);
    io.KeyMap[ImGuiKey_8] = KEY_TO_INT(Key::Num8);
    io.KeyMap[ImGuiKey_9] = KEY_TO_INT(Key::Num9);
    io.KeyMap[ImGuiKey_0] = KEY_TO_INT(Key::Num0);
    io.KeyMap[ImGuiKey_Minus] = KEY_TO_INT(Key::MinusSign);
    io.KeyMap[ImGuiKey_Equal] = KEY_TO_INT(Key::EqualSign);
    io.KeyMap[ImGuiKey_Backspace] = KEY_TO_INT(Key::Backspace);
    io.KeyMap[ImGuiKey_Tab] = KEY_TO_INT(Key::Tab);
    io.KeyMap[ImGuiKey_CapsLock] = KEY_TO_INT(Key::Capslock);
    io.KeyMap[ImGuiKey_LeftShift] = KEY_TO_INT(Key::LShift);
    io.KeyMap[ImGuiKey_LeftCtrl] = KEY_TO_INT(Key::LCtrl);
    io.KeyMap[ImGuiKey_LeftSuper] = KEY_TO_INT(Key::LWin);
    io.KeyMap[ImGuiKey_LeftAlt] = KEY_TO_INT(Key::LAlt);
    io.KeyMap[ImGuiKey_Space] = KEY_TO_INT(Key::Space);
    io.KeyMap[ImGuiKey_RightAlt] = KEY_TO_INT(Key::RAlt);
    io.KeyMap[ImGuiKey_RightSuper] = KEY_TO_INT(Key::RWin);
    io.KeyMap[ImGuiKey_RightCtrl] = KEY_TO_INT(Key::RCtrl);
    io.KeyMap[ImGuiKey_RightShift] = KEY_TO_INT(Key::RShift);
    io.KeyMap[ImGuiKey_Enter] = KEY_TO_INT(Key::Enter);
    io.KeyMap[ImGuiKey_A] = KEY_TO_INT(Key::A);
    io.KeyMap[ImGuiKey_B] = KEY_TO_INT(Key::B);
    io.KeyMap[ImGuiKey_C] = KEY_TO_INT(Key::C);
    io.KeyMap[ImGuiKey_D] = KEY_TO_INT(Key::D);
    io.KeyMap[ImGuiKey_E] = KEY_TO_INT(Key::E);
    io.KeyMap[ImGuiKey_F] = KEY_TO_INT(Key::F);
    io.KeyMap[ImGuiKey_G] = KEY_TO_INT(Key::G);
    io.KeyMap[ImGuiKey_H] = KEY_TO_INT(Key::H);
    io.KeyMap[ImGuiKey_I] = KEY_TO_INT(Key::I);
    io.KeyMap[ImGuiKey_J] = KEY_TO_INT(Key::J);
    io.KeyMap[ImGuiKey_K] = KEY_TO_INT(Key::K);
    io.KeyMap[ImGuiKey_L] = KEY_TO_INT(Key::L);
    io.KeyMap[ImGuiKey_M] = KEY_TO_INT(Key::M);
    io.KeyMap[ImGuiKey_N] = KEY_TO_INT(Key::N);
    io.KeyMap[ImGuiKey_O] = KEY_TO_INT(Key::O);
    io.KeyMap[ImGuiKey_P] = KEY_TO_INT(Key::P);
    io.KeyMap[ImGuiKey_Q] = KEY_TO_INT(Key::Q);
    io.KeyMap[ImGuiKey_R] = KEY_TO_INT(Key::R);
    io.KeyMap[ImGuiKey_S] = KEY_TO_INT(Key::S);
    io.KeyMap[ImGuiKey_T] = KEY_TO_INT(Key::T);
    io.KeyMap[ImGuiKey_U] = KEY_TO_INT(Key::U);
    io.KeyMap[ImGuiKey_V] = KEY_TO_INT(Key::V);
    io.KeyMap[ImGuiKey_W] = KEY_TO_INT(Key::W);
    io.KeyMap[ImGuiKey_X] = KEY_TO_INT(Key::X);
    io.KeyMap[ImGuiKey_Y] = KEY_TO_INT(Key::Y);
    io.KeyMap[ImGuiKey_Z] = KEY_TO_INT(Key::Z);
    io.KeyMap[ImGuiKey_LeftBracket] = KEY_TO_INT(Key::LBracket);
    io.KeyMap[ImGuiKey_RightBracket] = KEY_TO_INT(Key::RBracket);
    io.KeyMap[ImGuiKey_RightBracket] = KEY_TO_INT(Key::BackSlash);
    io.KeyMap[ImGuiKey_Semicolon] = KEY_TO_INT(Key::Semicolon);
    io.KeyMap[ImGuiKey_Apostrophe] = KEY_TO_INT(Key::Apostrophe);
    io.KeyMap[ImGuiKey_Comma] = KEY_TO_INT(Key::Comma);
    io.KeyMap[ImGuiKey_Period] = KEY_TO_INT(Key::Period);
    io.KeyMap[ImGuiKey_Slash] = KEY_TO_INT(Key::ForwardSlash);
    io.KeyMap[ImGuiKey_Delete] = KEY_TO_INT(Key::Delete);
    io.KeyMap[ImGuiKey_LeftArrow] = KEY_TO_INT(Key::LeftArrow);
    io.KeyMap[ImGuiKey_UpArrow] = KEY_TO_INT(Key::UpArrow);
    io.KeyMap[ImGuiKey_RightArrow] = KEY_TO_INT(Key::RightArrow);
    io.KeyMap[ImGuiKey_DownArrow] = KEY_TO_INT(Key::DownArrow);

#undef KEY_TO_INT
}

void UI::UpdateIO(float deltaTime)
{
    ImGuiIO& io = ImGui::GetIO();

    Window* window = Window::Get();

    io.DisplaySize.x = (float)window->GetClientAreaSize().x;
    io.DisplaySize.y = (float)window->GetClientAreaSize().y;
    io.DeltaTime = deltaTime;

    if (Input::IsMouseRaw() || !window->IsInFocus())
    {
        return;
    }

    // Update mouse
    io.AddMousePosEvent((float)Input::GetMousePos().x, (float)Input::GetMousePos().y);
    io.AddMouseButtonEvent(0, Input::IsKeyDown(Key::LButton));
    io.AddMouseButtonEvent(1, Input::IsKeyDown(Key::RButton));
    io.AddMouseWheelEvent(0.0f, Input::GetMouseWheelDelta());

    // Update typed characters
    const std::wstring& inputCharacters = Input::GetCharsTyped();

    for (wchar_t c : inputCharacters)
    {
        io.AddInputCharacterUTF16(c);
    }

    // Update keyboard
    for (int i = (int)Key::None + 1; i < (int)Key::KeysCount; i++)
    {
        io.KeysDown[i] = Input::IsKeyDown((Key)i);
    }
}

void UI::ResetInputs()
{
    ImGuiIO& io = ImGui::GetIO();

    io.DeltaTime = 0.0f;

    io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    io.AddMouseButtonEvent(0, false);
    io.AddMouseButtonEvent(1, false);
    io.AddMouseWheelEvent(0.0f, 0.0f);

    for (int i = (int)Key::None + 1; i < (int)Key::KeysCount; i++)
    {
        io.KeysDown[i] = false;
    }
}

void UI::BeginDock()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar();

    ImGui::DockSpace(ImGui::GetID(m_dockSpaceName.c_str()), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode);
}

void UI::EndDock()
{
    ImGui::End();
}

void UI::SetupLayout()
{
    /*

        -----------------------------------
        |          |            |         |
        | leftUp   |   middle   |         |
        |          |            |         |
        |----------|------------|  right  |
        |          |            |         |
        | leftDown |   bottom   |         |
        |          |            |         |
        -----------------------------------

        middleBottomRight = middle + bottom + right
        middleBottom = middle + bottom
        left = leftUp + leftDown

        */

    ImGuiID dockSpaceId = ImGui::GetID(m_dockSpaceName.c_str());

    ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode);

    ImGui::DockBuilderSetNodeSize(dockSpaceId, ImGui::GetWindowViewport()->Size);

    ImGuiID middleBottomRight = 0;
    ImGuiID left = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Left, 0.2f, nullptr, &middleBottomRight);

    ImVec2 sideSize{ ImGui::GetWindowViewport()->Size.x * 0.2f, ImGui::GetWindowViewport()->Size.y };
    ImGui::DockBuilderSetNodeSize(left, sideSize);

    ImGuiID leftUp = 0;
    ImGuiID leftDown = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.7f, nullptr, &leftUp);

    ImGui::DockBuilderSetNodeSize(leftUp, ImVec2(sideSize.x, sideSize.y * 0.3f));
    ImGui::DockBuilderSetNodeSize(leftDown, ImVec2(sideSize.x, sideSize.y * 0.7f));

    ImGuiID middleBottom = 0;
    ImGuiID right = ImGui::DockBuilderSplitNode(middleBottomRight, ImGuiDir_Right, 0.25f, nullptr, &middleBottom);

    ImGui::DockBuilderSetNodeSize(right, sideSize);

    ImGui::DockBuilderDockWindow(m_infoPanel.GetName().c_str(), leftUp);
    ImGui::DockBuilderDockWindow(m_statsPanel.GetName().c_str(), leftDown);
    ImGui::DockBuilderDockWindow(m_levelInspector.GetName().c_str(), right);

    ImGui::DockBuilderFinish(dockSpaceId);
}