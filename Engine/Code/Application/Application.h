#pragma once

#include <chrono>
#include <string_view>

#include <Engine/Engine.h>
#include <Window/Window.h>
#include <UI/UI.h>
#include <Level/Level.h>

class Application
{
public:
    Application(std::wstring_view applicationName);

    void Run();

    const std::filesystem::path& GetRootPath() const;

    Level* GetLevel();

    static Application* Get();

private:
    void RetrieveWorkingDirectory();

    void Terminate();

    void PreTick();
    void Tick();
    void PostTick();

    float CalculateDeltaTime();

private:
    std::filesystem::path m_rootPath;

    WindowPtr m_window;
    EnginePtr m_engine;
    UIPtr m_ui;
    LevelPtr m_level;

    std::chrono::steady_clock::time_point m_lastFrameTS;

    static Application* s_application;
};