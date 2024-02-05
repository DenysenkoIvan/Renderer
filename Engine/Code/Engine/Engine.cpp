#include "Engine.h"

#include <Input/Input.h>
#include <Application/Application.h>

Engine* Engine::s_engine = nullptr;

Engine::Engine()
{
    m_renderer = Renderer::Create();
    m_renderer->Initialize();
}

void Engine::PreTick()
{
    ProfileFunction();

    m_fpsStats.OnNewFrame();
    UpdateEngineStats();

    if (Window::Get()->IsResized())
    {
        m_renderer->Resize();
        Window::Get()->ResetIsResized();
    }
}

void Engine::OnTick(float deltaTime)
{
    ProfileFunction();

    UpdateCamera(deltaTime);

    const std::vector<Entity>& rootEntities = Application::Get()->GetLevel()->GetRootEntities();
    size_t rootEntitiesCount = rootEntities.size();
    for (int i = 0; i < rootEntitiesCount; i++)
    {
        RenderEntity(rootEntities[i], glm::mat4(1.0f));
    }
}

void Engine::PostTick()
{
    ProfileFunction();

    PerFrameData perFrame{};
    perFrame.projMatrix = m_camera.ProjMatrix();
    perFrame.viewMatrix = m_camera.ViewMatrix();
    perFrame.projViewMat = perFrame.projMatrix * perFrame.viewMatrix;
    
    // removing translation
    glm::mat4 rotation = perFrame.viewMatrix;
    rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    perFrame.invRotProjView = glm::inverse(perFrame.projMatrix * rotation);
    perFrame.cameraPosition = glm::vec4(m_camera.GetPosition(), 0.0f);
    perFrame.lightDirection = glm::vec4(glm::normalize(m_lightProps.lightDirection), 0.0f);
    perFrame.lightColorIntensity = glm::vec4(m_lightProps.lightColor, m_lightProps.lightIntensity);

    m_renderer->Render(perFrame);
}

void Engine::LoadSkybox(const std::filesystem::path& skyboxPath)
{
    m_renderer->LoadSkybox(skyboxPath);
}

const Camera& Engine::GetCamera() const
{
    return m_camera;
}

const EngineStats& Engine::GetStats() const
{
    return m_engineStats;
}

Renderer* Engine::GetRenderer()
{
    return m_renderer.get();
}

Engine* Engine::Get()
{
    return s_engine;
}

EnginePtr Engine::Create()
{
    Assert(s_engine == nullptr);

    EnginePtr engine = std::make_unique<Engine>();

    s_engine = engine.get();

    return engine;
}

void Engine::UpdateCamera(float deltaTime)
{
    if (Window::Get()->IsMinimized())
    {
        return;
    }

    const float moveSpeed = 10.0f;
    const float mouseSensetivity = 0.018f;
    if (Input::IsMouseRaw())
    {
        float mouseWheelDelta = Input::GetMouseWheelDelta();

        float distance = deltaTime * moveSpeed;
        float forwardDistance = distance + moveSpeed * mouseWheelDelta;
        if (Input::IsKeyDown(Key::W) || mouseWheelDelta)
        {
            m_camera.MoveForward(forwardDistance);
        }
        else if (Input::IsKeyDown(Key::S) || mouseWheelDelta)
        {
            m_camera.MoveForward(-forwardDistance);
        }

        if (Input::IsKeyDown(Key::A))
        {
            m_camera.MoveLeft(distance);
        }
        else if (Input::IsKeyDown(Key::D))
        {
            m_camera.MoveLeft(-distance);
        }

        if (Input::IsKeyDown(Key::Q))
        {
            m_camera.MoveUp(-distance);
        }
        else if (Input::IsKeyDown(Key::E))
        {
            m_camera.MoveUp(distance);
        }

        MousePos mouseDelta = Input::GetMousePosRawDelta();

        if (mouseDelta.x != 0)
        {
            m_camera.TurnLeft(mouseSensetivity * -mouseDelta.x);
        }
        if (mouseDelta.y != 0)
        {
            m_camera.TurnUp(mouseSensetivity * mouseDelta.y);
        }
    }

    if (Input::IsKeyPressed(Key::R))
    {
        if (Input::IsMouseRaw())
        {
            Window::Get()->ShowCursor();
        }
        else
        {
            Window::Get()->HideCursor();
        }
    }

    m_camera.SetAspectRatio(m_renderer->GetProps().GetSwapchainAspectRatio());
}

void Engine::UpdateEngineStats()
{
    m_engineStats.fps = m_fpsStats.GetFps();
    m_engineStats.frametime = m_fpsStats.GetFrametime();
}

void Engine::RenderEntity(Entity entity, const glm::mat4& parentTransform)
{
    TransformComponent& transformComponent = entity.GetComponent<TransformComponent>();
    if (entity.HasComponent<MeshComponent>())
    {
        MeshComponent& meshComponent = entity.GetComponent<MeshComponent>();

        meshComponent.Render(transformComponent.transform, parentTransform);
    }

    glm::mat4 currentTransform = parentTransform * entity.GetComponent<TransformComponent>().transform;

    const std::vector<Entity>& children = entity.GetChildren();
    size_t childrenCount = children.size();
    for (int i = 0; i < childrenCount; i++)
    {
        Entity child = children[i];

        RenderEntity(child, currentTransform);
    }
}