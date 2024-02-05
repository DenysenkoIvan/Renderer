#include "FpsStats.h"

FpsStats::FpsStats()
    : m_startTS(std::chrono::steady_clock::now())
    , m_period(std::chrono::milliseconds(500))
{
}

void FpsStats::SetPeriod(std::chrono::milliseconds updateTime)
{
    if (updateTime.count() != 0)
    {
        m_period = updateTime;
    }
}

void FpsStats::OnNewFrame()
{
    auto currTS = std::chrono::steady_clock::now();

    if (currTS >= m_startTS + m_period)
    {
        RecalculateFPS(currTS);
        m_startTS = currTS;
        m_frames = 0;
    }
    else
    {
        m_frames++;
    }
}

float FpsStats::GetFps() const
{
    return m_fps;
}

float FpsStats::GetFrametime() const
{
    return m_frametime;
}

void FpsStats::RecalculateFPS(const std::chrono::steady_clock::time_point& newTS)
{
    std::chrono::duration<float, std::milli> timePassed = newTS - m_startTS;

    float periodsPerSecond = 1000.0f / timePassed.count();
    m_fps = periodsPerSecond * m_frames;
    m_frametime = (float)timePassed.count() / m_frames;
}