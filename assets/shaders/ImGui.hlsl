#include "Common.hlsli"

struct ImGuiVertex
{
    float2 Position;
    float2 TC;
    float4 Color;
};

struct VertToPix
{
    float4 Position : SV_Position;
    float2 TC : TEXCOORD;
    float4 Color : COLOR;
};

struct PixOut
{
    float4 Color : SV_Target0;
};

struct PerFrameConstants
{
    float2 screenSizeInv;
};

struct DrawData
{
    ArrayBuffer constants;
    ArrayBuffer vertices;
    ArrayBuffer indices;
    Texture fontTexture;
    Sampler fontSampler;
};

PUSH_CONSTANTS(DrawData, drawData);

VertToPix MainVS(uint vtxID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VertToPix OUT = (VertToPix)0;

    uint vertexIndex = drawData.indices.Load<uint>(vtxID) + instanceID;

    ImGuiVertex IN = drawData.vertices.Load<ImGuiVertex>(vertexIndex);

    IN.Position *= drawData.constants.Load<PerFrameConstants>(0).screenSizeInv;

    OUT.Position = float4(ToDixectXCoordSystem((half2)IN.Position), 0.0f, 1.0f);
    OUT.TC = IN.TC;
    OUT.Color = IN.Color;

    return OUT;
}

PixOut MainPS(VertToPix IN)
{
    PixOut OUT = (PixOut)0;

    OUT.Color = drawData.fontTexture.Sample2D<float4>(drawData.fontSampler.Get(), IN.TC);
    OUT.Color *= IN.Color;

    return OUT;
}