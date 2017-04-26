#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#include "Shared.h"

namespace VulkanDemo
{
    class VulkanManager;

    class Window
    {
    public:
        Window(int width, int height);
        ~Window();

        bool Run();

        VkSwapchainKHR GetSwapchain() const;
        VkSurfaceFormatKHR const & GetFormat() const;
        VkSurfaceCapabilitiesKHR const & GetSurfaceCapabilities() const;
        std::vector<VkImage> const & GetImages() const;
        std::vector<VkImageView> const & GetImageViews() const;

    private:
        void CreateSystemWindow();
        void DestroySystemWindow();

        void Bind();
        void Unbind();

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
        VkSurfaceKHR                m_Surface = VK_NULL_HANDLE;
        VkSurfaceFormatKHR          m_Format{};
        VkSwapchainKHR              m_Swapchain = VK_NULL_HANDLE;
        std::vector<VkImage>        m_Images;
        std::vector<VkImageView>    m_ImageViews;

        VkSurfaceCapabilitiesKHR    m_SurfaceCapabilities{};
    };
} // VulkanDemo
