#include "StatsPanel.h"

void StatsPanel::SetFonts(ImFont* regularFont, ImFont* titleFont)
{
    Assert(regularFont && titleFont);

    m_regularFont = regularFont;
    m_titleFont = titleFont;
}

void StatsPanel::Update()
{
    const RendererStats& renderStats = Renderer::Get()->GetStats();

    float frameTimeSmooth = 0.0f;
    auto it = std::find(renderStats.gpuZones.begin(), renderStats.gpuZones.end(), "FRAME");
    if (it != renderStats.gpuZones.end())
    {
        frameTimeSmooth = std::lerp(m_prevGPUSmoothFrameTime, it->durationMilliseconds, 0.1f);
        m_prevGPUSmoothFrameTime = frameTimeSmooth;
    }

    ImGui::PushFont(m_titleFont);
    ImGui::Begin(m_name.c_str());

    ImGui::PushFont(m_regularFont);

    ImGui::Text("Draw calls: %d", renderStats.stats.drawCallCount);
    ImGui::Text("Dispatch calls: %d", renderStats.stats.dispatchCount);
    ImGui::Text("DrawMeshTasks calls: %d", renderStats.stats.drawMeshTasksCount);
    
    ImGui::Separator();

    const char* buttonText = m_isDisplayPipelineStats ? "Hide details" : "Show details";
    if (ImGui::Button(buttonText))
    {
        m_isDisplayPipelineStats = !m_isDisplayPipelineStats;
    }

    if (m_isDisplayPipelineStats && !renderStats.pipelineStatistics.empty())
    {
        ImGui::PushFont(m_titleFont);
        ImGui::Text("Pipeline Statistics");
        ImGui::PopFont();

        for (const PipelineStatistics& stat : renderStats.pipelineStatistics)
        {
            ImGui::Text("%s - %d", stat.name.c_str(), stat.count);
        }
    }

    if (!renderStats.gpuZones.empty())
    {
        ImGui::Separator();

        ImGui::PushFont(m_titleFont);
        ImGui::Text("GPU Zones");
        ImGui::PopFont();

        ImGui::Text("Frame (smooth): %.2fms", frameTimeSmooth);
        for (const GPUZone& zone : renderStats.gpuZones)
        {
            std::string zoneNameAdjusted(zone.depth * 2, ' ');
            zoneNameAdjusted += zone.name;
            ImGui::Text("%s: %.2fms", zoneNameAdjusted.c_str(), zone.durationMilliseconds);
        }
    }

    ImGui::PopFont();
    ImGui::PopFont();
    ImGui::End();
}

const std::string& StatsPanel::GetName() const
{
    return m_name;
}