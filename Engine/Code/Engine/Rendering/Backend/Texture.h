#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <Framework/Common.h>

#include "Common.h"
#include "ShaderUtils.h"
#include "VulkanImpl/VulkanTexture.h"

class RenderDriver;

class Texture;
using TexturePtr = std::shared_ptr<Texture>;

enum TextureUsageFlagBits : uint32_t
{
    TextureUsageNone              = 0,
    TextureUsageTransferSrc       = 1,
    TextureUsageTransferDst       = 2,
    TextureUsageSample            = 4,
    TextureUsageColorRenderTarget = 8,
    TextureUsageDepthRenderTarget = 16,
    TextureUsageStorage           = 32,
    TextureUsagePresent           = 64
};
using TextureUsageFlags = uint32_t;

struct TextureState
{
    TextureUsageFlags usage = 0;

    TextureState(TextureUsageFlags usage = 0)
        : usage(usage) {}

    bool IsUndefined() const
    {
        return usage == TextureUsageNone;
    }
};

class Texture
{
    friend class Swapchain;

    struct BoundLayer
    {
        int slot = -1;
        int layer = -1;
    };

public:
    NON_COPYABLE_MOVABLE(Texture);

    Texture() = default;
    Texture(TextureUsageFlags usage, Format format, int width, int height, int layerCount = 1, bool isUseMips = false);
    ~Texture();

    void SetName(std::string_view name);
    const std::string& GetName() const;

    int GetWidth() const { return m_texture.GetExtent3D().width; }
    int GetHeight() const { return m_texture.GetExtent3D().height; };
    int GetDepth() const { return m_texture.GetExtent3D().depth; };

    int GetMipCount() const { return m_texture.GetMipCount(); }
    int GetLayerCount() const { return m_texture.GetLayerCount(); }

    const TextureState& GetState() const { return m_state; }
    void SetState(const TextureState& state) { m_state = state; }

    int BindSRV();
    int BindUAV();
    int BindUAVLayer(int layer);

    VulkanTexture& GetTexture();

    static TexturePtr Create1D(TextureUsageFlags usage, Format format, int width, int layersCount = 1, bool isUseMips = false);
    static TexturePtr Create2D(TextureUsageFlags usage, Format format, int width, int height, int layersCount = 1, bool isUseMips = false);
    static TexturePtr LoadFromFile(const std::filesystem::path& path);

    static void ClearCache();

private:
    int CalculateMipCount(int width, int height);
    static std::unique_ptr<uint8_t[]> LoadToRAM(const std::filesystem::path& path, Format& format, int& width, int& height, int& channels, int& channelSize);

    void GetDescriptor(void* descriptor, int baseLayer, int layerCount, bool isStorage);

private:
    VulkanTexture m_texture;
    TextureUsageFlags m_usage = TextureUsageNone;
    TextureState m_state;

    int m_srvSlot = -1;
    int m_uavSlot = -1;
    std::vector<BoundLayer> m_uavSlotsLayers;

#if !defined(RELEASE_BUILD)
    std::string m_name;
#endif

    static std::unordered_map<std::filesystem::path, TexturePtr> s_textureCache;
};