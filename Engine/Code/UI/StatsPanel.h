#pragma once

#include <Framework/Common.h>
#include <Engine/Engine.h>

#include <imgui.h>

class StatsPanel
{
public:
    void SetFonts(ImFont* regularFont, ImFont* titleFont);

    void Update();

    const std::string& GetName() const;

private:
    std::string m_name = "GPU Stats";

    ImFont* m_regularFont = nullptr;
    ImFont* m_titleFont = nullptr;

    bool m_isDisplayPipelineStats = false;
    float m_prevGPUSmoothFrameTime = 0.0f;
};