#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#include "Shared.h"

#include "SceneRenderer.h"

namespace VulkanDemo
{
    class VulkanManager;

    class Window
    {
    public:
        Window(int width, int height);
        ~Window();

        bool Run();
        void Render();

    private:
        void CreateSystemWindow();
        void DestroySystemWindow();

        void Bind();
        void Unbind();

        void CreateRenderPass();
        void DestroyRenderPass();

        void CreateFramebuffers();
        void DestroyFramebuffers();

        void CreateSynchronization();
        void DestroySynchronization();

        void CreateCommandBuffer();
        void DestroyCommandBuffer();

        void WaitForCommandBuffer();

        static LRESULT CALLBACK WindowProc(
            HWND   hwnd,
            UINT   uMsg,
            WPARAM wParam,
            LPARAM lParam);

        int m_Width = 0;
        int m_Height = 0;

        HINSTANCE           m_AppInstance = NULL;
        const std::string   m_WindowClassName = "VulkanDemoWindowClass";
        HWND                m_WindowHandle = NULL;
        bool                m_IsClosed = false;

        // Renderer-specific variables:
        VulkanManager*              m_VulkanManager = nullptr;
        SceneRenderer               m_SceneRenderer;
        VkSurfaceKHR                m_Surface = VK_NULL_HANDLE;
        VkSurfaceFormatKHR          m_Format{};
        VkSwapchainKHR              m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage>        m_Images;
        std::vector<VkImageView>    m_ImageViews;

        VkSurfaceCapabilitiesKHR    m_SurfaceCapabilities{};

        VkRenderPass                m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer>  m_FrameBuffers;

        uint32_t            m_NextImageIndex = UINT32_MAX;

        VkSemaphore         m_ImageAcquiredSemaphore = VK_NULL_HANDLE;
        VkSemaphore         m_ImageRenderedSemaphore = VK_NULL_HANDLE;
        VkFence             m_CommandBufferProcessedFence = VK_NULL_HANDLE;

        VkCommandBuffer     m_CommandBuffer = VK_NULL_HANDLE;

        VkSemaphore         m_LastSceneRenderSemaphore = VK_NULL_HANDLE;
    };
} // VulkanDemo
