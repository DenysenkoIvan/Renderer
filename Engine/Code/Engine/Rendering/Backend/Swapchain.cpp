#include "Swapchain.h"

Swapchain::Swapchain()
{
    m_swapchain.Create();

    GatherTextures();
}

Swapchain::~Swapchain()
{}

void Swapchain::Resize()
{
    m_swapchain.Resize();

    GatherTextures();
}

bool Swapchain::IsRenderable()
{
    return m_swapchain.IsRenderable();
}

int Swapchain::GetImageCount()
{
    return m_swapchain.GetImageCount();
}

TexturePtr Swapchain::GetTexture()
{
    if (IsRenderable())
    {
        return m_textures[m_swapchain.GetImageIndex()];
    }
    else
    {
        return nullptr;
    }
}

glm::uvec2 Swapchain::GetSize() const
{
    VkExtent2D extent = m_swapchain.GetExtent();

    return glm::uvec2(extent.width, extent.height);
}

void Swapchain::GatherTextures()
{
    m_textures.clear();

    VkExtent2D swapchainExtent = m_swapchain.GetExtent();
    VkExtent3D extent3D{ swapchainExtent.width, swapchainExtent.height, 1 };

    for (uint32_t i = 0; i < m_swapchain.GetImageCount(); i++)
    {
        TexturePtr texture = std::make_shared<Texture>();
        texture->m_texture.Create(m_swapchain.GetImage(i), VK_IMAGE_TYPE_2D, m_swapchain.GetSurfaceFormat().format, extent3D, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, m_swapchain.GetImageUsage(), VK_SHARING_MODE_EXCLUSIVE);

        std::string textureName = std::format("$SwapchainImage{}", i);
        texture->SetName(textureName);

        m_textures.push_back(texture);
    }
}

VulkanSwapchain& Swapchain::GetVkSwapchain()
{
    return m_swapchain;
}

SwapchainPtr Swapchain::Create()
{
    return std::make_unique<Swapchain>();
}