#include "Common.h"

#include <Framework/Assert.h>

size_t GetFormatSize(Format format)
{
    switch (format)
    {
    case Format::R8_UINT:
        return 1 * 1;
    case Format::RGBA8_UNORM:
        return 4 * 1;
    case Format::RGBA8_SRGB:
        return 4 * 1;
    case Format::RG16_UNORM:
        return 2 * 2;
    case Format::RG16_SFLOAT:
        return 2 * 2;
    case Format::RGBA16_SFLOAT:
        return 4 * 2;
    case Format::R32_UINT:
        return 1 * 4;
    case Format::RG32_SFLOAT:
        return 2 * 4;
    case Format::RGBA32_SFLOAT:
        return 4 * 4;
    case Format::B10G11R11_UFLOAT:
        return 4;
    case Format::D32_SFLOAT:
        return 1 * 4;
    default:
        Assert(false);
        return 1;
    }
}

bool IsFormatHasDepth(Format format)
{
    if (format == Format::D32_SFLOAT)
    {
        return true;
    }
    else
    {
        return false;
    }
}