#pragma once

#include <Windows.h>
#include <string>

#include "Shared.h"

class Renderer;

class Window
{
public:
    Window(int width, int height, Renderer* renderer);
    ~Window();

    bool Run();

private:
    void Open();
    void Close();

    void Bind();
    void Unbind();

    static LRESULT CALLBACK WindowProc(
        HWND   hwnd,
        UINT   uMsg,
        WPARAM wParam,
        LPARAM lParam);

    int m_Width     = 0;
    int m_Height    = 0;

    HINSTANCE           m_AppInstance       = NULL;
    const std::string   m_WindowClassName   = "VulkanDemoWindowClass";
    HWND                m_WindowHandle      = NULL;

    // Renderer-specific variables:
    Renderer*           m_Renderer          = nullptr;
    VkSurfaceKHR        m_Surface           = VK_NULL_HANDLE;
    VkSwapchainKHR      m_Swapchain         = VK_NULL_HANDLE;
};
