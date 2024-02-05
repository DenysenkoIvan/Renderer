#include "Common.hlsli"

struct DrawData
{
    int width;
    int height;
    float widthInv;
    float heightInv;
    RWTexture blurTexture;
    RWTexture tempTexture;
    int isHorizontalBlur;
};

PUSH_CONSTANTS(DrawData, drawData);

float3 SampleTexture(RWTexture texture, int2 pixel)
{
    pixel.x = clamp(pixel.x, 0, drawData.width - 1);
    pixel.y = clamp(pixel.y, 0, drawData.height - 1);

    return texture.Load2D<float4>(pixel).rgb;
}

float3 Blur9Pixels(RWTexture texture, int2 pixel, int2 offset)
{
    float3 o = 0.0f;

    o += SampleTexture(texture, pixel + offset * -4);
    o += SampleTexture(texture, pixel + offset * -3) * 8.0f;
    o += SampleTexture(texture, pixel + offset * -2) * 28.0f;
    o += SampleTexture(texture, pixel - offset     ) * 56.0f;
    o += SampleTexture(texture, pixel              ) * 70.0f;
    o += SampleTexture(texture, pixel + offset     ) * 56.0f;
    o += SampleTexture(texture, pixel + offset *  2) * 28.0f;
    o += SampleTexture(texture, pixel + offset *  3) * 8.0f;
    o += SampleTexture(texture, pixel + offset *  4);

    return o / 256.0f;
}

[numthreads(8, 8, 1)]
void MainCS(uint3 DTid : SV_DispatchThreadID)
{
    int2 pixel = DTid.xy;
    if (pixel.x < 0 || pixel.x >= drawData.width || pixel.y < 0 || pixel.y >= drawData.height)
    {
        return;
    }

    if (drawData.isHorizontalBlur != 0)
    {
        // Horizontal blur
        float3 o = Blur9Pixels(drawData.blurTexture, pixel, int2(1, 0));

        drawData.tempTexture.Store2D<float4>(pixel, float4(o, 1.0f));
    }
    else
    {
        // Vertical Blur
        float3 o = Blur9Pixels(drawData.tempTexture, pixel, int2(0, 1));

        drawData.blurTexture.Store2D<float4>(pixel, float4(o, 1.0f));
    }
}