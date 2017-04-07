#include "Window.h"

#include <cassert>

#include <Windows.h>
#include <array>

#include "Renderer.h"

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
        window->Unbind();
        window->Close();
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

Window::Window(int width, int height, Renderer* renderer)
{
    assert(width > 0);
    assert(height > 0);
    assert(renderer != nullptr);

    m_Width = width;
    m_Height = height;
    m_Renderer = renderer;

    m_AppInstance = GetModuleHandle(NULL);

    Open();
    Bind();
}

Window::~Window()
{
    assert(m_WindowHandle == NULL);

    m_Renderer = nullptr;
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

void Window::Bind()
{
    VkPhysicalDevice physicalDevice = m_Renderer->GetPhysicalDevice();
    uint32_t queueFamily = m_Renderer->GetGraphicsQueueFamilyIndex();

    // Query for Win32 WSI support.
    VkBool32 queueFamilyCanPresentToWindows = vkGetPhysicalDeviceWin32PresentationSupportKHR(
        physicalDevice,
        m_Renderer->GetGraphicsQueueFamilyIndex()
    );
    if (!queueFamilyCanPresentToWindows)
    {
        Fail("The selected graphics queue family doesn't support presentation to the Microsoft Windows desktop", -1);
    }

    // Create the surface.
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType         = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.flags         = 0;
    surfaceCreateInfo.pNext         = NULL;
    surfaceCreateInfo.hinstance     = m_AppInstance;
    surfaceCreateInfo.hwnd          = m_WindowHandle;
    CheckResult(vkCreateWin32SurfaceKHR(m_Renderer->GetInstance(), &surfaceCreateInfo, NULL, &m_Surface));

    // Query for WSI support for the surface.
    VkBool32 wsiSupported;
    CheckResult(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamily, m_Surface, &wsiSupported));
    if (!wsiSupported)
    {
        Fail("The selected graphics queue family doesn't support presentation to the specified surface", -1);
    }

    // Get the supported surface capabilities.
    VkSurfaceCapabilitiesKHR supportedSurfaceCapabilities;
    CheckResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &supportedSurfaceCapabilities));

    // Select the number of images.
    uint32_t selectedImageCount = supportedSurfaceCapabilities.minImageCount + 1;
    if (supportedSurfaceCapabilities.maxImageCount > 0)
    {
        selectedImageCount = min(selectedImageCount, supportedSurfaceCapabilities.maxImageCount);
    }

    // Get the supported surface formats.
    uint32_t supportedSurfaceFormatsCount;
    CheckResult(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &supportedSurfaceFormatsCount, NULL));
    assert(supportedSurfaceFormatsCount > 0 && "No supported surface formats.");
    std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats{ supportedSurfaceFormatsCount };
    CheckResult(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &supportedSurfaceFormatsCount, supportedSurfaceFormats.data()));

    // Select a surface format.
    VkSurfaceFormatKHR selectedSurfaceFormat{};
    selectedSurfaceFormat.format = VK_FORMAT_B8G8R8_UNORM;
    selectedSurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    for (auto const & supportedSurfaceFormat : supportedSurfaceFormats)
    {
        if (supportedSurfaceFormat.format != VK_FORMAT_UNDEFINED)
        {
            selectedSurfaceFormat = supportedSurfaceFormat;
            break;
        }
    }

    // Get the present modes.
    uint32_t supportedPresentModesCount;
    CheckResult(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &supportedPresentModesCount, NULL));
    assert(supportedPresentModesCount > 0 && "No supported presentation mode.");
    std::vector<VkPresentModeKHR> supportedPresentModes{ supportedPresentModesCount };
    CheckResult(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &supportedPresentModesCount, supportedPresentModes.data()));

    // Select a present mode.
    VkPresentModeKHR selectedPresentMode = supportedPresentModes[0];
    for (auto supportedPresentMode : supportedPresentModes)
    {
        if (supportedPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            selectedPresentMode = supportedPresentMode;
            break;
        }
    }

    // Create the swapchain.
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType                   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext                   = NULL;
    swapchainCreateInfo.flags                   = 0;
    swapchainCreateInfo.surface                 = m_Surface;
    swapchainCreateInfo.minImageCount           = selectedImageCount;
    swapchainCreateInfo.imageFormat             = selectedSurfaceFormat.format;
    swapchainCreateInfo.imageColorSpace         = selectedSurfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent             = supportedSurfaceCapabilities.currentExtent;
    swapchainCreateInfo.imageArrayLayers        = 1; // Non-stereoscopic-3D
    swapchainCreateInfo.imageUsage              = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode        = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount   = 1;
    swapchainCreateInfo.pQueueFamilyIndices     = &queueFamily;
    swapchainCreateInfo.preTransform            = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha          = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode             = selectedPresentMode;
    swapchainCreateInfo.clipped                 = VK_TRUE;
    swapchainCreateInfo.oldSwapchain            = VK_NULL_HANDLE;
    CheckResult(vkCreateSwapchainKHR(m_Renderer->GetDevice(), &swapchainCreateInfo, NULL, &m_Swapchain));
}

void Window::Unbind()
{
    vkDestroySwapchainKHR(m_Renderer->GetDevice(), m_Swapchain, NULL);
    m_Swapchain = VK_NULL_HANDLE;

    vkDestroySurfaceKHR(m_Renderer->GetInstance(), m_Surface, NULL);
    m_Surface = VK_NULL_HANDLE;
}
