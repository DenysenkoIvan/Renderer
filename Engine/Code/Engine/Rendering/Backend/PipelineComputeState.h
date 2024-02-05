#pragma once

#include "Shader.h"

class PipelineComputeState
{
public:
    void SetShader(Shader* shader);
    Shader* GetShader() const;

    void Reset();

private:
    Shader* m_shader = nullptr;
};