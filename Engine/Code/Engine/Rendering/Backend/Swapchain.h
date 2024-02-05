#pragma once

#include "VulkanImpl/VulkanSwapchain.h"

#include "Texture.h"

class Swapchain;
using SwapchainPtr = std::unique_ptr<Swapchain>;

class Swapchain
{
public:
    NON_COPYABLE_MOVABLE(Swapchain);

    Swapchain();
    ~Swapchain();

    void Resize();

    bool IsRenderable();

    int GetImageCount();
    TexturePtr GetTexture();

    glm::uvec2 GetSize() const;

    void GatherTextures();

    VulkanSwapchain& GetVkSwapchain();

    static SwapchainPtr Create();

private:
    VulkanSwapchain m_swapchain;
    std::vector<TexturePtr> m_textures;
};