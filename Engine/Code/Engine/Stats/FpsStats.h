#pragma once

#include <chrono>

class FpsStats
{
public:
    FpsStats();

    void SetPeriod(std::chrono::milliseconds updateTime);

    void OnNewFrame();

    float GetFps() const;
    float GetFrametime() const;

private:
    void RecalculateFPS(const std::chrono::steady_clock::time_point& newTS);

private:
    int m_frames = 1;
    float m_fps = 1;
    float m_frametime = 0.0f;
    std::chrono::milliseconds m_period;
    std::chrono::steady_clock::time_point m_startTS;
};