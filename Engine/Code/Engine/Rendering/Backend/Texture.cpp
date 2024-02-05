#include "Texture.h"

#include <Framework/Common.h>
#include <Engine/Engine.h>

#include <stb_image.h>
#include <stb_image_resize2.h>

#include "DescriptorBindingTable.h"

#include "VulkanImpl/VulkanHelpers.h"

std::unordered_map<std::filesystem::path, TexturePtr> Texture::s_textureCache;

Texture::Texture(TextureUsageFlags usage, Format format, int width, int height, int layerCount, bool isUseMips)
    : m_usage(usage)
{
    VkImageUsageFlags vkUsage = 0;
    if (usage & TextureUsageTransferSrc)
    {
        vkUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (usage & TextureUsageTransferDst)
    {
        vkUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (usage & TextureUsageSample)
    {
        vkUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (usage & TextureUsageColorRenderTarget)
    {
        vkUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (usage & TextureUsageDepthRenderTarget)
    {
        vkUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (usage & TextureUsageStorage)
    {
        vkUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    int mipCount = 1;
    if (isUseMips)
    {
        mipCount = CalculateMipCount(width, height);
    }

    VkImageType imageType = VK_IMAGE_TYPE_2D;
    if (height == 1)
    {
        imageType = VK_IMAGE_TYPE_1D;
    }

    VkExtent3D extent3D{ (uint32_t)width, (uint32_t)height, 1 };
    uint32_t graphicsQueueIndex = VkContext::Get()->GetDevice().GetGraphicsQueueIndex();

    m_texture.Create(imageType, (VkFormat)format, extent3D, mipCount, layerCount,
        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, vkUsage, VK_SHARING_MODE_EXCLUSIVE,
        1, &graphicsQueueIndex, VK_IMAGE_LAYOUT_UNDEFINED);
}

Texture::~Texture()
{
    if (m_srvSlot != -1)
    {
        DescriptorBindingTable::Get()->ReleaseSampledImageSlot(m_srvSlot);
        m_srvSlot = -1;
    }
    if (m_uavSlot != -1)
    {
        DescriptorBindingTable::Get()->ReleaseStorageImageSlot(m_uavSlot);
        m_uavSlot = -1;
    }

    for (BoundLayer& boundLayer : m_uavSlotsLayers)
    {
        DescriptorBindingTable::Get()->ReleaseStorageImageSlot(boundLayer.slot);
    }
    m_uavSlotsLayers.clear();
}

void Texture::SetName(std::string_view name)
{
#if !defined(RELEASE_BUILD)
    m_name = name;

    VkDebugUtilsObjectNameInfoEXT nameInfo{ .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
    nameInfo.objectHandle = (uint64_t)m_texture.GetVkImage();
    nameInfo.pObjectName = m_name.c_str();

    VK_VALIDATE(VkContext::Get()->GetProcAddresses().vkSetDebugUtilsObjectName(VkContext::Get()->GetVkDevice(), &nameInfo));
#endif
}

const std::string& Texture::GetName() const
{
#if !defined(RELEASE_BUILD)
    return m_name;
#else
    static std::string empty;
    return empty;
#endif
}

int Texture::BindSRV()
{
    if (m_srvSlot != -1)
    {
        return m_srvSlot;
    }

    if ((m_usage & TextureUsageSample) == 0)
    {
        Assert(false, "Texture should have SAMPLE usage");
        return -1;
    }

    uint8_t descriptor[64]{};

    GetDescriptor(descriptor, 0, GetLayerCount(), false);

    m_srvSlot = DescriptorBindingTable::Get()->GetSampledImageFreeSlot(descriptor);

    Assert(m_srvSlot != -1);

    return m_srvSlot;
}

int Texture::BindUAV()
{
    if (m_uavSlot != -1)
    {
        return m_uavSlot;
    }

    if ((m_usage & TextureUsageStorage) == 0)
    {
        Assert(false, "Texture should have STORAGE usage");
        return -1;
    }

    uint8_t descriptor[64]{};

    GetDescriptor(descriptor, 0, GetLayerCount(), true);

    m_uavSlot = DescriptorBindingTable::Get()->GetStorageImageFreeSlot(descriptor);

    Assert(m_uavSlot != -1);

    return m_uavSlot;
}

int Texture::BindUAVLayer(int layer)
{
    Assert(layer >= 0 && layer <= GetLayerCount());

    if (GetLayerCount() == 1)
    {
        return BindUAV();
    }

    auto it = std::find_if(m_uavSlotsLayers.begin(), m_uavSlotsLayers.end(), [layer](BoundLayer& boundLayer)
        {
            return boundLayer.layer == layer;
        });

    if (it != m_uavSlotsLayers.end())
    {
        return it->slot;
    }

    uint8_t descriptor[64]{};
    GetDescriptor(descriptor, layer, 1, true);

    BoundLayer boundLayer{};
    boundLayer.layer = layer;
    boundLayer.slot = DescriptorBindingTable::Get()->GetStorageImageFreeSlot(descriptor);

    m_uavSlotsLayers.push_back(boundLayer);

    return boundLayer.slot;
}

VulkanTexture& Texture::GetTexture()
{
    return m_texture;
}

TexturePtr Texture::Create1D(TextureUsageFlags usage, Format format, int width, int layersCount, bool isUseMips)
{
    return std::make_shared<Texture>(usage, format, width, 1, layersCount, isUseMips);
}

TexturePtr Texture::Create2D(TextureUsageFlags usage, Format format, int width, int height, int layersCount, bool isUseMips)
{
    return std::make_shared<Texture>(usage, format, width, height, layersCount, isUseMips);
}

TexturePtr Texture::LoadFromFile(const std::filesystem::path& path)
{
    if (s_textureCache.contains(path))
    {
        return s_textureCache[path];
    }

    std::string asciiPath = path.string();

    Format format = Format::Undefined;
    int width = 0, height = 0;
    int channels = 0;
    int channelSize = 1; // Bytes

    std::unique_ptr<uint8_t[]> data = LoadToRAM(path, format, width, height, channels, channelSize);
    if (!data)
    {
        return nullptr;
    }

    bool isUseMips = channelSize == 1;

    CommandBufferPtr cmdBuffer = Renderer::Get()->GetLoadCmdBuffer();

    TextureUsageFlags srcUsage = isUseMips ? TextureUsageTransferSrc : 0;
    TexturePtr texture = Texture::Create2D(TextureUsageTransferDst | TextureUsageSample | srcUsage, format, width, height, 1, isUseMips);
    texture->SetName(asciiPath);
    cmdBuffer->CopyToTexture(texture, data.get(), width * height * 4 * channelSize);

    if (isUseMips)
    {
        cmdBuffer->GenerateMipmaps(texture);
    }

    s_textureCache.emplace(path, texture);

    return texture;
}

void Texture::ClearCache()
{
    s_textureCache.clear();
}

int Texture::CalculateMipCount(int width, int height)
{
    return (int)std::floor(std::log2((float)std::max(width, height))) + 1;
}

std::unique_ptr<uint8_t[]> Texture::LoadToRAM(const std::filesystem::path& path, Format& format, int& width, int& height, int& channels, int& channelSize)
{
    std::string asciiPath = path.string();

    bool isSRGB = false;
    if (asciiPath.find("BaseColor", 0) != std::string::npos)
    {
        isSRGB = true;
    }

    void* data = nullptr;
    bool isHdrFormat = false;
    if (path.extension() == ".hdr")
    {
        isHdrFormat = true;
        channelSize = 4;
        format = Format::RGBA32_SFLOAT;
        data = stbi_loadf(asciiPath.c_str(), &width, &height, &channels, 4);
    }
    else
    {
        channelSize = 1;
        format = isSRGB ? Format::RGBA8_SRGB : Format::RGBA8_UNORM;
        data = stbi_load(asciiPath.c_str(), &width, &height, &channels, 4);
    }

    Assert(std::popcount(std::bit_cast<uint32_t>(width)) == 1 && std::popcount(std::bit_cast<uint32_t>(height)) == 1);

    if (!data)
    {
        LogError("Failed to load texture {}", asciiPath);
        format = Format::Undefined;
        width = 0;
        height = 0;
        channels = 0;
        channelSize = 0;
        return nullptr;
    }

    std::unique_ptr<uint8_t[]> result = nullptr;

    if (!isHdrFormat && (width > 2048 || height > 2048))
    {
        int outputWidth = 0;
        int outputHeight = 0;
        
        if (width >= height)
        {
            outputWidth = 2048;
            outputHeight = (int)(2048 * ((float)height / width));
        }
        else
        {
            outputHeight = 2048;
            outputWidth = (int)(2048 * ((float)width / height));
        }

        result = std::make_unique<uint8_t[]>(outputWidth * outputHeight * channelSize * 4);

        void* res = nullptr;
        if (channelSize == 1)
        {
            res = stbir_resize_uint8_linear((const unsigned char*)data, width, height, 0, result.get(), outputWidth, outputHeight, 0, STBIR_RGBA);
        }
        else if (channelSize == 4)
        {
            res = stbir_resize_float_linear((const float*)data, width, height, 0, (float*)result.get(), outputWidth, outputHeight, 0, STBIR_RGBA);
        }

        Assert(res);

        width = outputWidth;
        height = outputHeight;
    }
    else
    {
        result = std::make_unique<uint8_t[]>(width * height * channelSize * 4);
        memcpy(result.get(), data, width * height * channelSize * 4);
    }

    stbi_image_free(data);

    return result;
}

void Texture::GetDescriptor(void* descriptor, int baseLayer, int layerCount, bool isStorage)
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = TextureUsageToLayout(isStorage ? TextureUsageStorage : TextureUsageSample);
    if (baseLayer == 0 && layerCount == 6 && GetLayerCount() == 6)
    {
        imageInfo.imageView = m_texture.GetViewCube(0, GetMipCount());
    }
    else
    {
        imageInfo.imageView = m_texture.GetView2D(baseLayer, 0, GetMipCount());
    }

    VkDescriptorGetInfoEXT info{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
    if (isStorage)
    {
        info.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        info.data.pStorageImage = &imageInfo;
    }
    else
    {
        info.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        info.data.pSampledImage = &imageInfo;
    }

    int descriptorSize = isStorage ? DescriptorBindingTable::Get()->GetStorageImageDescriptorSize()  : DescriptorBindingTable::Get()->GetSampledImageDescriptorSize();

    VkContext::Get()->GetProcAddresses().vkGetDescriptor(VkContext::Get()->GetVkDevice(), &info, descriptorSize, descriptor);
}