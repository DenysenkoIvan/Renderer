#include "Common.hlsli"

struct AppToVert
{
    float2 Position : SV_Position;
};

struct VertToPix
{
    float4 Position : SV_Position;
    float2 TC       : TEXCOORD;
};

struct PixOut
{
    float4 Color : SV_Target0;
};

VertToPix mainVS(AppToVert IN)
{
    VertToPix OUT = (VertToPix)0;

    OUT.Position = float4(ToDixectXCoordSystem(IN.Position), 0.0f, 1.0f);
    OUT.TC = IN.Position;

    return OUT;
}

Texture2D<float4> tex0 : register(t0);
SamplerState sampler0 : register(s0);

PixOut mainPS(VertToPix IN)
{
    PixOut OUT = (PixOut)0;

    float2 circleCenter = float2(0.5f, 0.5f);
    float circleRadius = 0.3f;

    float4 circleColor = float4(0.953f, 0.871, 0.305, 1.0f);
    float4 backgroundColor = float4(0.578f, 0.574f, 0.594f, 1.0f);

    if (distance(IN.TC.xy, circleCenter) < circleRadius)
    {
        OUT.Color = circleColor;
    }
    else
    {
        OUT.Color = tex0.Sample(sampler0, IN.TC);
        OUT.Color.rgb *= OUT.Color.a;
    }

    return OUT;
}