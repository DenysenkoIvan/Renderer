#pragma once

#include <cstdint>

enum class Format : uint32_t
{
    Undefined = 0,
    R8_UINT = 13,
    RGBA8_UNORM = 37,
    RGBA8_SRGB = 43,
    RG16_UNORM = 77,
    RG16_SFLOAT = 83,
    RGBA16_SFLOAT = 97,
    R32_UINT = 98,
    RG32_SFLOAT = 103,
    RGBA32_SFLOAT = 109,
    B10G11R11_UFLOAT = 122,
    D32_SFLOAT = 126
};

size_t GetFormatSize(Format format);

bool IsFormatHasDepth(Format format);

enum class CompareOp : uint8_t
{
    Never = 0,
    Less = 1,
    Equal = 2,
    LessOrEqual = 3,
    Greater = 4,
    NotEqual = 5,
    GreaterOrEqual = 6,
    Always = 7
};

enum class CullMode : uint8_t
{
    None = 0,
    Front = 1,
    Back = 2,
    FrontAndBack = 3
};

enum class FrontFace : uint8_t
{
    CounterClockwise = 0,
    Clockwise = 1
};

enum class BlendFactor : uint8_t
{
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha
};

enum class BlendOp : uint8_t
{
    Add = 0,
    Substract,
    ReverseSubstract,
    Min,
    Max
};