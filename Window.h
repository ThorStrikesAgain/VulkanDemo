#pragma once

#include <Windows.h>
#include <string>

class Window
{
public:
    Window(int width, int height);
    ~Window();

    bool Run();

private:
    void Open();
    void Close();

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
};
