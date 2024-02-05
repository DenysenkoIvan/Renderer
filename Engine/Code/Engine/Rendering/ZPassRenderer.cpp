#include "ZPassRenderer.h"

#include <algorithm>

void ZPassRenderer::Create(const CommonRenderResources* commonResources, const RendererProperties* props)
{
    m_commonResources = commonResources;
    m_renderProps = props;
}

void ZPassRenderer::NewFrame()
{
    m_stats.Reset();
}

void ZPassRenderer::RenderZPrepass(std::vector<RenderObjectPtr>& renderObjects, CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    cmdBuffer->MarkerBegin("ZPREPASS");
    cmdBuffer->BeginZone("ZPREPASS");

    TexturePtr depthTarget = m_commonResources->depthTarget;

    cmdBuffer->ResetBindAndRenderStates();

    cmdBuffer->SetDepthTargetClear(depthTarget, 1.0f, 0);
    cmdBuffer->BeginRenderPass();

    cmdBuffer->SetViewport(0.0f, 0.0f, (float)depthTarget->GetWidth(), (float)depthTarget->GetHeight(), 0.0f, 1.0f);
    cmdBuffer->SetScissor(0, 0, depthTarget->GetWidth(), depthTarget->GetHeight());

    for (RenderObjectPtr& rd : renderObjects)
    {
        const MaterialPtr& material = rd->material;
        if (material->props.alphaMode != AlphaMode::Opaque)
        {
            continue;
        }

        if (!Input::IsKeyDown(Key::F))
        {
            const PSOGraphics* pso = rd->GetPSO(DrawCallType::ZPrePass);
            if (!pso)
            {
                pso = CreateZPrePassDrawCallPSO(rd, cmdBuffer);

                if (!pso)
                {
                    continue;
                }
            }

            struct DrawData
            {
                int positions;
                int indices;
                int perFrameBuffer;
                int perInstanceBuffer;
            };

            DrawData drawData{};
            drawData.positions = rd->mesh->positions->BindSRV();
            drawData.indices = rd->mesh->indexBuffer ? rd->mesh->indexBuffer->BindSRV() : 0;
            drawData.perFrameBuffer = m_commonResources->perFrameBuffer->BindSRV();
            drawData.perInstanceBuffer = rd->perInstanceBuffer->BindSRV();

            cmdBuffer->PushConstants(&drawData, sizeof(drawData));

            cmdBuffer->RegisterSRVUsageBuffer(rd->mesh->positions);
            cmdBuffer->RegisterSRVUsageBuffer(rd->mesh->indexBuffer);
            cmdBuffer->RegisterSRVUsageBuffer(m_commonResources->perFrameBuffer);
            cmdBuffer->RegisterSRVUsageBuffer(rd->perInstanceBuffer);

            cmdBuffer->BindPsoGraphics(pso);

            m_stats.drawCallCount++;
            cmdBuffer->Draw((uint32_t)rd->mesh->indexBuffer->GetSize() / sizeof(uint32_t));
        }
        else
        {
            const PSOGraphics* pso = rd->GetPSO(DrawCallType::ZPrePassMesh);
            if (!pso)
            {
                pso = CreateZPrePassMeshDrawCallPSO(rd, cmdBuffer);

                if (!pso)
                {
                    continue;
                }
            }

            cmdBuffer->BindPsoGraphics(pso);

            m_stats.drawCallCount++;
            cmdBuffer->DrawMeshTasks((rd->mesh->meshletsCount + 31) / 32, 1, 1);
        }
    }

    cmdBuffer->EndRenderPass();

    cmdBuffer->EndZone();
    cmdBuffer->MarkerEnd();
}

void ZPassRenderer::RenderZPass(std::vector<RenderObjectPtr>& renderObjects, CommandBufferPtr& cmdBuffer, bool isOpaque)
{
    ProfileFunction();
    
    if (renderObjects.empty())
    {
        return;
    }

    std::string_view markerName = isOpaque ? "ZPASS" : "TRANSPARENT_PASS";
    cmdBuffer->MarkerBegin(markerName.data());
    cmdBuffer->BeginZone(markerName.data());

    TexturePtr hdrTarget = m_commonResources->hdrTarget;
    cmdBuffer->SetViewport(0.0f, 0.0f, (float)hdrTarget->GetWidth(), (float)hdrTarget->GetHeight(), 0.0f, 1.0f);
    cmdBuffer->SetScissor(0, 0, hdrTarget->GetWidth(), hdrTarget->GetHeight());

    for (RenderObjectPtr& rd : renderObjects)
    {
        struct DrawData
        {
            int isUseBackFaceCull;
            int indices;
            int vertices;
            int meshlets;
            uint32_t meshletCount;
            int meshletIndices;
            int meshletVertices;
            int materialProps;
            int perFrameBuffer;
            int perInstanceBuffer;
        };

        DrawData drawData{};
        drawData.isUseBackFaceCull = (int)!static_cast<bool>(rd->material->props.isDoubleSided);
        drawData.indices = rd->mesh->indexBuffer ? rd->mesh->indexBuffer->BindSRV() : 0;
        drawData.vertices = rd->mesh->vertices->BindSRV();
        drawData.meshlets = rd->mesh->meshlets->BindSRV();
        drawData.meshletCount = rd->mesh->meshletsCount;
        drawData.meshletIndices = rd->mesh->meshletTriangles->BindSRV();
        drawData.meshletVertices = rd->mesh->meshletVertices->BindSRV();
        drawData.materialProps = rd->material->propsBuffer->BindSRV();
        drawData.perFrameBuffer = m_commonResources->perFrameBuffer->BindSRV();
        drawData.perInstanceBuffer = rd->perInstanceBuffer->BindSRV();

        cmdBuffer->RegisterSRVUsageBuffer(rd->mesh->indexBuffer);
        cmdBuffer->RegisterSRVUsageBuffer(rd->mesh->vertices);
        cmdBuffer->RegisterSRVUsageBuffer(rd->mesh->meshlets);
        cmdBuffer->RegisterSRVUsageBuffer(rd->material->propsBuffer);
        cmdBuffer->RegisterSRVUsageBuffer(m_commonResources->perFrameBuffer);
        cmdBuffer->RegisterSRVUsageBuffer(rd->perInstanceBuffer);

        cmdBuffer->RegisterSRVUsageTexture(rd->material->albedoTexture);
        cmdBuffer->RegisterSRVUsageTexture(rd->material->normalsTexture);
        cmdBuffer->RegisterSRVUsageTexture(rd->material->metRoughTexture);
        cmdBuffer->RegisterSRVUsageTexture(rd->material->emissiveTexture);
        cmdBuffer->RegisterSRVUsageTexture(rd->material->specularTexture);
        cmdBuffer->RegisterSRVUsageTexture(rd->material->occlusionTexture);

        if (!Input::IsKeyDown(Key::F))
        {
            const PSOGraphics* pso = rd->GetPSO(DrawCallType::ZPass);
            if (!pso)
            {
                pso = CreateZPassDrawCallPSO(rd, cmdBuffer);

                if (!pso)
                {
                    continue;
                }
            }

            cmdBuffer->BindPsoGraphics(pso);

            cmdBuffer->PushConstants(&drawData, sizeof(drawData));

            m_stats.drawCallCount++;
            cmdBuffer->Draw((uint32_t)rd->mesh->indexBuffer->GetSize() / sizeof(uint32_t));
        }
        else
        {
            const PSOGraphics* pso = rd->GetPSO(DrawCallType::ZPassMesh);
            if (!pso)
            {
                pso = CreateZPassMeshDrawCallPSO(rd, cmdBuffer);

                if (!pso)
                {
                    continue;
                }
            }

            cmdBuffer->BindPsoGraphics(pso);

            m_stats.drawCallCount++;
            cmdBuffer->DrawMeshTasks((rd->mesh->meshletsCount + 31) / 32, 1, 1);
        }
    }

    cmdBuffer->EndZone();
    cmdBuffer->MarkerEnd();
}

const RenderStats& ZPassRenderer::GetStats() const
{
    return m_stats;
}

const PSOGraphics* ZPassRenderer::CreateZPrePassDrawCallPSO(RenderObjectPtr& renderObject, CommandBufferPtr& cmdBuffer)
{
    MeshPtr& mesh = renderObject->mesh;
    MaterialPtr& material = renderObject->material;

    Shader* shader = Shader::GetGraphics("assets/shaders/ZPrepass.hlsl");

    PipelineGraphicsState state{};
    state.SetShader(shader);
    state.GetDepthStencilState().SetDepthTest(true, CompareOp::LessOrEqual);
    state.GetRasterizationState().SetCull(CullMode::Front, FrontFace::Clockwise);

    cmdBuffer->PropagateRenderingInfo(state);

    const PSOGraphics* pso = PSOGraphics::Get(state);
    if (!pso)
    {
        return nullptr;
    }

    renderObject->drawCallsPSOs[DrawCallType::ZPrePass] = pso;

    return pso;
}

const PSOGraphics* ZPassRenderer::CreateZPrePassMeshDrawCallPSO(RenderObjectPtr& renderObject, CommandBufferPtr& cmdBuffer)
{
    MeshPtr& mesh = renderObject->mesh;
    MaterialPtr& material = renderObject->material;

    ShaderDefines shaderDefines;
    shaderDefines.Add("USE_MESH_SHADING");

    Shader* shader = Shader::GetGraphics("assets/shaders/ZPrepass.hlsl", shaderDefines, true);

    PipelineGraphicsState state{};
    state.SetShader(shader);
    state.GetDepthStencilState().SetDepthTest(true, CompareOp::LessOrEqual);
    state.GetRasterizationState().SetCull(CullMode::Front, FrontFace::Clockwise);

    cmdBuffer->PropagateRenderingInfo(state);

    const PSOGraphics* pso = PSOGraphics::Get(state);
    if (!pso)
    {
        return nullptr;
    }

    renderObject->drawCallsPSOs[DrawCallType::ZPrePassMesh] = pso;

    return pso;
}

const PSOGraphics* ZPassRenderer::CreateZPassDrawCallPSO(RenderObjectPtr& renderObject, CommandBufferPtr& cmdBuffer)
{
    MeshPtr& mesh = renderObject->mesh;
    MaterialPtr& material = renderObject->material;

    ShaderDefines shaderDefines;
    if (mesh->components & VertexComponentTangentBitangents)
    {
        shaderDefines.Add("USE_TANGENTS_BITANGENTS");
    }
    if (mesh->components & VertexComponentUvs)
    {
        shaderDefines.Add("USE_UV");
    }
    if (mesh->components & VertexComponentColors)
    {
        shaderDefines.Add("USE_VERTEX_COLOR");
    }

    Shader* shader = Shader::GetGraphics("assets/shaders/ZPass.hlsl", shaderDefines);

    PipelineGraphicsState state{};
    state.SetShader(shader);

    bool depthWrite = true;
    if (material->props.alphaMode == AlphaMode::Blend)
    {
        depthWrite = false;
        state.GetBlendState().SetBlendingColor(0, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendOp::Add);
        state.GetBlendState().SetBlendingAlpha(0, BlendFactor::Zero, BlendFactor::OneMinusSrcAlpha, BlendOp::Add);
    }
    state.GetDepthStencilState().SetDepthTest(depthWrite, CompareOp::LessOrEqual);
    
    if (material->props.isDoubleSided)
    {
        state.GetRasterizationState().SetCull(CullMode::None, FrontFace::CounterClockwise);
    }
    else
    {
        state.GetRasterizationState().SetCull(CullMode::Front, FrontFace::Clockwise);
    }
    
    if (material->props.albedo.a <= 0.999f)
    {
        state.GetBlendState().SetBlendingColor(0, BlendFactor::OneMinusSrcAlpha, BlendFactor::SrcAlpha, BlendOp::Add);
    }

    cmdBuffer->PropagateRenderingInfo(state);

    const PSOGraphics* pso = PSOGraphics::Get(state);
    if (!pso)
    {
        return nullptr;
    }

    renderObject->drawCallsPSOs[DrawCallType::ZPass] = pso;

    return pso;
}

const PSOGraphics* ZPassRenderer::CreateZPassMeshDrawCallPSO(RenderObjectPtr& renderObject, CommandBufferPtr& cmdBuffer)
{
    MeshPtr& mesh = renderObject->mesh;
    MaterialPtr& material = renderObject->material;

    ShaderDefines shaderDefines;
    if (mesh->components & VertexComponentTangentBitangents)
    {
        shaderDefines.Add("USE_TANGENTS_BITANGENTS");
    }
    if (mesh->components & VertexComponentUvs)
    {
        shaderDefines.Add("USE_UV");
    }
    if (mesh->components & VertexComponentColors)
    {
        shaderDefines.Add("USE_VERTEX_COLOR");
    }
    shaderDefines.Add("USE_MESH_SHADING");

    Shader* shader = Shader::GetGraphics("assets/shaders/ZPass.hlsl", shaderDefines, true);

    PipelineGraphicsState state{};
    state.SetShader(shader);

    bool depthWrite = true;
    if (material->props.alphaMode == AlphaMode::Blend)
    {
        depthWrite = false;
        state.GetBlendState().SetBlendingColor(0, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendOp::Add);
        state.GetBlendState().SetBlendingAlpha(0, BlendFactor::Zero, BlendFactor::OneMinusSrcAlpha, BlendOp::Add);
    }
    state.GetDepthStencilState().SetDepthTest(depthWrite, CompareOp::LessOrEqual);

    if (material->props.isDoubleSided)
    {
        state.GetRasterizationState().SetCull(CullMode::None, FrontFace::CounterClockwise);
    }
    else
    {
        state.GetRasterizationState().SetCull(CullMode::Front, FrontFace::Clockwise);
    }

    if (material->props.albedo.a <= 0.999f)
    {
        state.GetBlendState().SetBlendingColor(0, BlendFactor::OneMinusSrcAlpha, BlendFactor::SrcAlpha, BlendOp::Add);
    }

    cmdBuffer->PropagateRenderingInfo(state);

    const PSOGraphics* pso = PSOGraphics::Get(state);
    if (!pso)
    {
        return nullptr;
    }

    renderObject->drawCallsPSOs[DrawCallType::ZPassMesh] = pso;

    return pso;
}