#pragma once

#include <Framework/Common.h>
#include <Engine/Engine.h>

#include <imgui.h>

class InfoPanel
{
public:
    void SetFonts(ImFont* regularFont, ImFont* titleFont);

    void Update();

    const std::string& GetName() const;

private:
    std::string m_name = "Info";

    ImFont* m_regularFont = nullptr;
    ImFont* m_titleFont = nullptr;
};