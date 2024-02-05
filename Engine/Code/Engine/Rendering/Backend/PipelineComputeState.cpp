#include "PipelineComputeState.h"

void PipelineComputeState::SetShader(Shader* shader)
{
    m_shader = shader;
}

Shader* PipelineComputeState::GetShader() const
{
    return m_shader;
}

void PipelineComputeState::Reset()
{
    m_shader = nullptr;
}