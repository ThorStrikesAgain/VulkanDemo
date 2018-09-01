#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#include "Shared.h"

#include "SceneRenderer.h"

namespace VulkanDemo
{
    class BlitPipelineGenerator;
    class VulkanManager;

    class Window
    {
    public:
        Window(int width, int height);
        ~Window();

        bool Run();
        void Render();

    private:
        class CommandBufferPool
        {
        public:
            CommandBufferPool(int size);
            ~CommandBufferPool();

            void GetNextAvailable(VkCommandBuffer* commandBuffer, VkFence* fence);

        private:
            VulkanManager*               m_VulkanManager = nullptr;

            std::vector<VkCommandBuffer> m_CommandBuffers;
            std::vector<VkFence>         m_Fences;

            int m_Size = 0;
            int m_Current = 0;
        };

        void CreateSystemWindow();
        void DestroySystemWindow();

        void Bind();
        void Unbind();

        void CreateRenderPass();
        void DestroyRenderPass();

        void CreatePipeline();
        void DestroyPipeline();

        void CreateFramebuffers();
        void DestroyFramebuffers();

        void CreateSynchronization();
        void DestroySynchronization();

        void CreateDescriptorSet();
        void DestroyDescriptorSet();

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
        uint32_t                    m_ImageCount = 0;

        VkSurfaceCapabilitiesKHR    m_SurfaceCapabilities{};

        VkRenderPass                m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer>  m_FrameBuffers;

        uint32_t            m_NextImageIndex = UINT32_MAX;

        VkSemaphore         m_ImageAcquiredBeforeBlitSemaphore = VK_NULL_HANDLE;
        VkSemaphore         m_BlitCompletedBeforePresentSemaphore = VK_NULL_HANDLE;
        VkSemaphore         m_BlitCompletedBeforeSceneRenderSemaphore = VK_NULL_HANDLE;
        bool                m_BlitCompletedBeforeSceneRendererSemaphoreUsed = false;


        VkSemaphore         m_SceneRenderedBeforeBlitSemaphore = VK_NULL_HANDLE;

        BlitPipelineGenerator*      m_PipelineGenerator = nullptr;
        VkDescriptorSet             m_BlitDescriptorSet = VK_NULL_HANDLE;

        CommandBufferPool*          m_CommandBufferPool = nullptr;
    };
} // VulkanDemo
