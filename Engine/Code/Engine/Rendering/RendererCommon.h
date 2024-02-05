#pragma once

#include <glm/glm.hpp>

struct RenderStats
{
    int drawCallCount = 0;
    int dispatchCount = 0;
    int drawMeshTasksCount = 0;

    void Reset()
    {
        drawCallCount = 0;
        dispatchCount = 0;
        drawMeshTasksCount = 0;
    }

    RenderStats& operator+=(const RenderStats& other)
    {
        drawCallCount += other.drawCallCount;
        dispatchCount += other.dispatchCount;
        drawMeshTasksCount += other.drawMeshTasksCount;
        return *this;
    }
};

struct RendererProperties
{
    size_t frameId = 0;
    glm::uvec2 renderResolution = glm::uvec2(1280, 720);
    glm::uvec2 swapchainResolution = glm::uvec2(0, 0);

    bool isUseZPrepass = false;

    float GetRenderAspectRatio() const
    {
        return (float)renderResolution.x / (float)renderResolution.y;
    }

    float GetSwapchainAspectRatio() const
    {
        return (float)swapchainResolution.x / (float)swapchainResolution.y;
    }
};