#pragma once

#include <Framework/Common.h>

#include <Level/Entity.h>
#include <Window/Window.h>

#include "Rendering/Renderer.h"
#include "Stats/FpsStats.h"
#include "Stats/EngineStats.h"

#include "Camera.h"

class Engine;
using EnginePtr = std::unique_ptr<Engine>;

class Engine
{
public:
    Engine();

    void PreTick();
    void OnTick(float deltaTime);
    void PostTick();

    void LoadSkybox(const std::filesystem::path& skyboxPath);

    const Camera& GetCamera() const;

    const EngineStats& GetStats() const;

    Renderer* GetRenderer();

    static Engine* Get();
    static EnginePtr Create();

private:
    void UpdateCamera(float deltaTime);
    void UpdateEngineStats();
    void RenderEntity(Entity entity, const glm::mat4& parentTransform);

private:
    struct LightsData
    {
        glm::vec3 lightDirection = glm::vec3(20.0f, 10.0f, -10.0f);
        glm::vec3 lightColor = glm::vec3(1.0f);
        float lightIntensity = 10.0f;
    };

private:
    RendererPtr m_renderer;
    FpsStats m_fpsStats;
    EngineStats m_engineStats;

    Camera m_camera;
    
    LightsData m_lightProps;

    static Engine* s_engine;
};