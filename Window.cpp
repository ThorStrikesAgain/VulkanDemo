#include "Window.h"

#include <cassert>

#include <Windows.h>

LRESULT CALLBACK Window::WindowProc(
    HWND   hwnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    Window * window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_CLOSE:
        window->Close();
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

Window::Window(int width, int height)
{
    m_Width = width;
    m_Height = height;
    m_AppInstance = GetModuleHandle(NULL);

    Open();
}

Window::~Window()
{
    m_AppInstance = NULL;
}

bool Window::Run()
{
    if (m_WindowHandle == NULL)
    {
        return false;
    }

    MSG msg;
    if (PeekMessage(&msg, m_WindowHandle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}

void Window::Open()
{
    // Create the window class.
    // TODO: The class should be unique and should be reused. For simplicity, we'll temporarily assume that only one
    // window is ever created.
    WNDCLASSEX windowClassSettings{};
    windowClassSettings.cbSize = sizeof(WNDCLASSEX);
    windowClassSettings.style = CS_HREDRAW | CS_VREDRAW;
    windowClassSettings.lpfnWndProc = WindowProc;
    windowClassSettings.cbClsExtra = 0;
    windowClassSettings.cbWndExtra = 0;
    windowClassSettings.hInstance = m_AppInstance;
    windowClassSettings.hIcon = NULL;
    windowClassSettings.hCursor = NULL;
    windowClassSettings.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    windowClassSettings.lpszMenuName = NULL;
    windowClassSettings.lpszClassName = m_WindowClassName.c_str();
    windowClassSettings.hIconSm = NULL;

    ATOM windowClassHandle = RegisterClassEx(&windowClassSettings);
    assert(windowClassHandle != 0 && "Failed to register the window class.");

    // Determine the window style. This will impact the required window size.
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    
    // Calculate the adjusted window rect required in order to have a renderable area that matches what we want.
    // (0,0) is the top left corner of the window. The horizontal axis increases to the right and the vertical axis
    // increases downwards.
    RECT rect{};
    rect.right = m_Width;
    rect.bottom = m_Height;
    BOOL succeeded = AdjustWindowRect(
        &rect,  // lpRect
        style,  // dwStyle
        false   // bMenu
    );
    assert(succeeded && "Failed to compute the dimensions to use.");

    // Create the window.
    m_WindowHandle = CreateWindowEx(
        0,                          // dwExStyle,
        m_WindowClassName.c_str(),  // lpClassName
        "Vulkan Demo",              // lpWindowName
        style,                      // dwStyle
        CW_USEDEFAULT,              // x
        CW_USEDEFAULT,              // y
        rect.right - rect.left,     // nWidth
        rect.bottom - rect.top,     // nHeight
        NULL,                       // hWndParent
        NULL,                       // hMenu
        m_AppInstance,              // hInstance
        NULL                        // lpParam
    );
    assert(m_WindowHandle != NULL);
    
    SetWindowLongPtr(m_WindowHandle, GWLP_USERDATA, (LONG_PTR)this);
    
    ShowWindow(m_WindowHandle, SW_SHOWNORMAL);
}

void Window::Close()
{
    DestroyWindow(m_WindowHandle);
    UnregisterClass(m_WindowClassName.c_str(), m_AppInstance);
    m_WindowHandle = NULL;
}
