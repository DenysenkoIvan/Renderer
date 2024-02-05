#include "Application.h"

#include <Framework/Common.h>

#include <algorithm>

Application* Application::s_application = nullptr;

Application::Application(std::wstring_view applicationName)
{
    s_application = this;

    RetrieveWorkingDirectory();

    m_window = Window::Create(applicationName, 1280, 720);
    m_engine = Engine::Create();
    m_ui = UI::Create();
    m_level = Level::Create();

    m_lastFrameTS = std::chrono::steady_clock::now();
}

void Application::Run()
{
    m_window->SetFocused();

    while (true)
    {
        ProfileFrame();
        ProfileScope("Main Loop");

        m_window->PollEvents();

        if (m_window->IsClosed())
        {
            break;
        }

        PreTick();
        Tick();
        PostTick();
    }

    Terminate();
}

const std::filesystem::path& Application::GetRootPath() const
{
    return m_rootPath;
}

Level* Application::GetLevel()
{
    return m_level.get();
}

Application* Application::Get()
{
    return s_application;
}

void Application::RetrieveWorkingDirectory()
{
    std::wstring rootPath = std::filesystem::current_path();
    std::replace(rootPath.begin(), rootPath.end(), L'\\', L'/');
    m_rootPath = rootPath;
}

void Application::Terminate()
{
    m_level = nullptr;
    m_ui = nullptr;
    m_engine = nullptr;
    m_window = nullptr;
}

void Application::PreTick()
{
    m_engine->PreTick();
}

void Application::Tick()
{
    float deltaTime = CalculateDeltaTime();

    m_engine->OnTick(deltaTime);
    m_ui->OnTick(deltaTime);
}

void Application::PostTick()
{
    m_engine->PostTick();
}

float Application::CalculateDeltaTime()
{
    auto currTS = std::chrono::steady_clock::now();
    float delta = std::chrono::duration<float>(currTS - m_lastFrameTS).count();
    m_lastFrameTS = currTS;

    return delta;
}