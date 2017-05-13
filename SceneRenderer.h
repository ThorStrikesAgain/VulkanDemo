#pragma once

#include "Shared.h"

namespace VulkanDemo
{
    class Scene;
    class VulkanManager;

    ///
    /// Renders a scene using the forward rendering technique.
    ///
    class SceneRenderer
    {
    public:
        SceneRenderer(VkImageLayout colorLayout);
        ~SceneRenderer();

        VkImage Render(const Scene * scene, int width, int height);

    private:
        void CreateForwardRenderPass();
        void DestroyForwardRenderpass();

        void UpdateFramebuffer(int width, int height);
        void CreateFramebuffer(int width, int height);
        void DestroyFramebuffer();

        void CreateCommandBuffer();
        void DestroyCommandBuffer();

        VulkanManager * m_VulkanManager = nullptr;

        bool m_IsInitialized = false;
        int m_Width = -1;
        int m_Height = -1;
        VkImageLayout m_ColorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkRenderPass m_ForwardRenderPass = VK_NULL_HANDLE;

        VkImage m_ForwardColorImage = VK_NULL_HANDLE;
        VkDeviceMemory m_ForwardColorMemory = VK_NULL_HANDLE;
        VkImageView m_ForwardColorImageView = VK_NULL_HANDLE;

        VkImage m_ForwardDepthStencilImage = VK_NULL_HANDLE;
        VkDeviceMemory m_ForwardDepthStencilMemory = VK_NULL_HANDLE;
        VkImageView m_ForwardDepthStencilImageView = VK_NULL_HANDLE;

        VkFramebuffer m_ForwardFramebuffer = VK_NULL_HANDLE;

        VkCommandBuffer m_CommandBuffer = NULL;
    };
} // VulkanDemo
