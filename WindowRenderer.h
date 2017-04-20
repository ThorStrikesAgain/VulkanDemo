#pragma once

#include <vector>

#include "Shared.h"

namespace VulkanDemo
{
    class VulkanManager;
    class Window;

    class WindowRenderer
    {
    public:
        WindowRenderer(VulkanManager* vulkanManager, Window* window);
        ~WindowRenderer();

        void Render(VkImageView src);

    private:
        void CreateRenderPass();
        void DestroyRenderPass();

        void CreateFramebuffers();
        void DestroyFramebuffers();

        void CreateSynchronization();
        void DestroySynchronization();

        void CreateCommandBuffer();
        void DestroyCommandBuffer();

        void WaitForCommandBuffer();

        VulkanManager*   m_VulkanManager = nullptr;
        Window*          m_Window = nullptr;

        VkRenderPass                m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer>  m_FrameBuffers;

        uint32_t            m_NextImageIndex = UINT32_MAX;

        VkSemaphore         m_ImageAcquiredSemaphore = VK_NULL_HANDLE;
        VkSemaphore         m_ImageRenderedSemaphore = VK_NULL_HANDLE;
        VkFence             m_CommandBufferProcessedFence = VK_NULL_HANDLE;
        bool                m_CommandBufferPending = false;

        VkCommandBuffer     m_CommandBuffer = VK_NULL_HANDLE;
    };
} // VulkanDemo
