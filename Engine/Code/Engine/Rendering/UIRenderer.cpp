#include "UIRenderer.h"

#include <imgui.h>

#include <glm/gtc/packing.hpp>

#include "Renderer.h"

struct ImGuiPerFrameConstants
{
    glm::vec2 screenSizeInv;
};

void UIRenderer::Create(const RendererProperties* props)
{
    ProfileFunction();

    m_rendererProps = props;

    m_perFrameBuffer = Buffer::CreateStructured(sizeof(ImGuiPerFrameConstants), false);
}

void UIRenderer::NewFrame()
{
    m_stats.Reset();
}

void UIRenderer::Render(CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();
    
    if (!Prepare())
    {
        return;
    }

    if (!m_pso)
    {
        CreatePso(cmdBuffer);
    }

    if (!m_pso || !m_uiVtx.get() || !m_uiIdx.get())
    {
        return;
    }

    ImDrawData* imDrawData = ImGui::GetDrawData();
    if (!imDrawData->CmdListsCount || !imDrawData->Valid)
    {
        return;
    }

    cmdBuffer->MarkerBegin("UI");
    cmdBuffer->BeginZone("UI");

    cmdBuffer->ResetBindAndRenderStates();

    cmdBuffer->SetViewport(0.0f, 0.0f, (float)m_rendererProps->swapchainResolution.x, (float)m_rendererProps->swapchainResolution.y, 0.0f, 1.0f);
    cmdBuffer->SetScissor(0, 0, m_rendererProps->swapchainResolution.x, m_rendererProps->swapchainResolution.y);

    cmdBuffer->BindPsoGraphics(m_pso);

    struct DrawData
    {
        int constantsBuffer;
        int verticesBuffer;
        int indicesBuffer;
        int fontTexture;
        int fontSampler;
    };

    DrawData drawData{};
    drawData.constantsBuffer = m_perFrameBuffer->BindSRV();
    drawData.verticesBuffer = m_uiVtx->BindSRV();
    drawData.indicesBuffer = m_uiIdx->BindSRV();
    drawData.fontTexture = m_fontsTexture->BindSRV();
    drawData.fontSampler = Sampler::GetLinear().GetBindSlot();

    cmdBuffer->PushConstants(&drawData, sizeof(drawData));

    cmdBuffer->RegisterSRVUsageBuffer(m_perFrameBuffer);
    cmdBuffer->RegisterSRVUsageBuffer(m_uiVtx);
    cmdBuffer->RegisterSRVUsageBuffer(m_uiIdx);
    cmdBuffer->RegisterSRVUsageTexture(m_fontsTexture);

    uint32_t vtxOffset = 0;
    uint32_t idxOffset = 0;
    for (int i = 0; i < imDrawData->CmdListsCount; i++)
    {
        const ImVec2& displayPos = imDrawData->DisplayPos;

        ImDrawList* cmdList = imDrawData->CmdLists[i];

        for (const ImDrawCmd& drawCmd : cmdList->CmdBuffer)
        {
            const ImVec4& clipRect = drawCmd.ClipRect;

            int scissor[4]
            {
                (int)(clipRect.x - displayPos.x),
                (int)(clipRect.y - displayPos.y),
                (int)(clipRect.z - displayPos.x - scissor[0]),
                (int)(clipRect.w - displayPos.y - scissor[1])
            };

            cmdBuffer->SetScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
            cmdBuffer->Draw(drawCmd.ElemCount, 1, idxOffset + drawCmd.IdxOffset, vtxOffset + drawCmd.VtxOffset);

            m_stats.drawCallCount++;
        }

        vtxOffset += cmdList->VtxBuffer.Size;
        idxOffset += cmdList->IdxBuffer.Size;
    }

    cmdBuffer->SetScissor(0, 0, m_rendererProps->swapchainResolution.x, m_rendererProps->swapchainResolution.y);

    cmdBuffer->EndZone();
    cmdBuffer->MarkerEnd();
}

const RenderStats& UIRenderer::GetStats() const
{
    return m_stats;
}

void UIRenderer::CreatePso(CommandBufferPtr& cmdBuffer)
{
    Shader* shader = Shader::GetGraphics("assets/shaders/ImGui.hlsl");
    if (!shader)
    {
        return;
    }

    PipelineGraphicsState state{};
    state.SetShader(shader);

    state.GetRenderingState();
    state.GetBlendState().SetBlendingColor(0, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendOp::Add);
    state.GetBlendState().SetBlendingAlpha(0, BlendFactor::One, BlendFactor::OneMinusSrcAlpha, BlendOp::Add);

    cmdBuffer->PropagateRenderingInfo(state);

    m_pso = PSOGraphics::Get(state);
}

bool UIRenderer::Prepare()
{
    ProfileFunction();

    CommandBufferPtr cmdBuffer = Renderer::Get()->GetLoadCmdBuffer();

    cmdBuffer->MarkerBegin("UI PRE_RENDER");

    if (!ImGui::GetIO().Fonts->IsBuilt())
    {
        CreateFontsTexture(cmdBuffer);
        cmdBuffer->MarkerEnd();
        return false;
    }

    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();

    if (!drawData->CmdListsCount || !drawData->Valid)
    {
        cmdBuffer->MarkerEnd();
        return false;
    }

    struct ImDrawVertPadded
    {
        float pos[2];
        float uv[2];
        float color[4];
    };

    bool isVtxBufferRecreated = false;
    if (!m_uiVtx.get() || !m_uiVtx || m_vtxCount < drawData->TotalVtxCount)
    {
        m_vtxCount = drawData->TotalVtxCount;
        isVtxBufferRecreated = true;
        m_uiVtx = Buffer::CreateStructured(sizeof(ImDrawVertPadded) * drawData->TotalVtxCount, false);
    }
    if (!m_uiIdx.get() || m_idxCount < drawData->TotalIdxCount)
    {
        m_uiIdx = Buffer::CreateStructured(sizeof(uint32_t) * drawData->TotalIdxCount, false);
        m_idxCount = drawData->TotalIdxCount;
    }

    std::vector<ImDrawVert> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(drawData->TotalVtxCount);
    indices.reserve(drawData->TotalIdxCount);

    for (int i = 0; i < drawData->CmdListsCount; i++)
    {
        ImDrawList* cmdList = drawData->CmdLists[i];

        vertices.insert(vertices.end(), cmdList->VtxBuffer.begin(), cmdList->VtxBuffer.end());
        indices.insert(indices.end(), cmdList->IdxBuffer.begin(), cmdList->IdxBuffer.end());
    }

    std::vector<ImDrawVertPadded> verticesConverted;
    for (const ImDrawVert& vert : vertices)
    {
        ImDrawVertPadded newVert;
        newVert.pos[0] = vert.pos[0];
        newVert.pos[1] = vert.pos[1];
        newVert.uv[0] = vert.uv[0];
        newVert.uv[1] = vert.uv[1];

        uint32_t color = vert.col;
        float r = (float)(color & 255) / 255.0f;
        float g = (float)((color >> 8) & 255) / 255.0f;
        float b = (float)((color >> 16) & 255) / 255.0f;
        float a = (float)((color >> 24) & 255) / 255.0f;

        newVert.color[0] = r;
        newVert.color[1] = g;
        newVert.color[2] = b;
        newVert.color[3] = a;

        verticesConverted.push_back(newVert);
    }

    cmdBuffer->CopyToBuffer(m_uiVtx, verticesConverted.data(), verticesConverted.size() * sizeof(ImDrawVertPadded));
    cmdBuffer->CopyToBuffer(m_uiIdx, indices.data(), indices.size() * sizeof(uint32_t));

    ImGuiPerFrameConstants perFrameConstants{ glm::vec2(1.0f / m_rendererProps->swapchainResolution.x, 1.0f / m_rendererProps->swapchainResolution.y) };

    cmdBuffer->CopyToBuffer(m_perFrameBuffer, &perFrameConstants, sizeof(ImGuiPerFrameConstants));

    cmdBuffer->MarkerEnd();

    return true;
}

void UIRenderer::CreateFontsTexture(CommandBufferPtr& cmdBuffer)
{
    ProfileFunction();

    uint8_t* pixels = nullptr;
    int width = 1, height = 1, bytesPerPixel = 1;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);

    cmdBuffer->MarkerBegin("CREATE FONTS TEXTURE");

    m_fontsTexture = Texture::Create2D(TextureUsageTransferDst | TextureUsageSample, Format::RGBA8_UNORM, width, height);
    cmdBuffer->CopyToTexture(m_fontsTexture, pixels, width * height * bytesPerPixel);
    cmdBuffer->RegisterSRVUsageTexture(m_fontsTexture);

    cmdBuffer->MarkerEnd();
}