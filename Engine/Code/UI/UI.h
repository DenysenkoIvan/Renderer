#pragma once

#include <imgui.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "InfoPanel.h"
#include "LevelInspector.h"
#include "StatsPanel.h"

class UI;
using UIPtr = std::unique_ptr<UI>;

class UI
{
public:
    UI();

    void OnTick(float deltaTime);

    static UI* Get();
    static UIPtr Create();

private:
    void LoadFonts();
    void RemapKeys();

    void UpdateIO(float deltaTime);
    void ResetInputs();

    void BeginDock();
    void EndDock();

    void SetupLayout();

private:
    std::string m_dockSpaceName = "RootDockSpace";

    ImFont* m_regularFont = nullptr;
    ImFont* m_titleFont = nullptr;

    bool m_isShowUi = true;

    InfoPanel m_infoPanel;
    LevelInspector m_levelInspector;
    StatsPanel m_statsPanel;

    static UI* s_ui;
};