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
        window->m_IsClosed = true;
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

    CreateSystemWindow();
    Bind();
}

Window::~Window()
{
    Unbind();
    DestroySystemWindow();

    m_Renderer = nullptr;
    m_AppInstance = NULL;
}

bool Window::Run()
{
    if (m_IsClosed)
    {
        return false;
    }

    MSG msg;
    if (PeekMessage(&msg, m_WindowHandle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return !m_IsClosed;
}

VkSwapchainKHR Window::GetSwapchain() const
{
    return m_Swapchain;
}

VkSurfaceFormatKHR const & Window::GetFormat() const
{
    return m_Format;
}

VkSurfaceCapabilitiesKHR const & Window::GetSurfaceCapabilities() const
{
    return m_SurfaceCapabilities;
}

std::vector<VkImage> const & Window::GetImages() const
{
    return m_Images;
}

std::vector<VkImageView> const & Window::GetImageViews() const
{
    return m_ImageViews;
}

void Window::CreateSystemWindow()
{
    // Create the window class.
    // TODO: The class should be unique and should be reused. For simplicity, we'll temporarily assume that only one
    // window is ever created.
    WNDCLASSEX windowClassSettings{};
    windowClassSettings.cbSize          = sizeof(WNDCLASSEX);
    windowClassSettings.style           = CS_HREDRAW | CS_VREDRAW;
    windowClassSettings.lpfnWndProc     = WindowProc;
    windowClassSettings.cbClsExtra      = 0;
    windowClassSettings.cbWndExtra      = 0;
    windowClassSettings.hInstance       = m_AppInstance;
    windowClassSettings.hIcon           = NULL;
    windowClassSettings.hCursor         = NULL;
    windowClassSettings.hbrBackground   = GetSysColorBrush(COLOR_WINDOW);
    windowClassSettings.lpszMenuName    = NULL;
    windowClassSettings.lpszClassName   = m_WindowClassName.c_str();
    windowClassSettings.hIconSm         = NULL;

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

void Window::DestroySystemWindow()
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
    CheckResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &m_SurfaceCapabilities));

    // Select the number of images.
    uint32_t selectedImageCount = m_SurfaceCapabilities.minImageCount + 1;
    if (m_SurfaceCapabilities.maxImageCount > 0)
    {
        selectedImageCount = min(selectedImageCount, m_SurfaceCapabilities.maxImageCount);
    }

    // Get the supported surface formats.
    uint32_t supportedSurfaceFormatsCount;
    CheckResult(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &supportedSurfaceFormatsCount, NULL));
    assert(supportedSurfaceFormatsCount > 0 && "No supported surface formats.");
    std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats{ supportedSurfaceFormatsCount };
    CheckResult(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &supportedSurfaceFormatsCount, supportedSurfaceFormats.data()));

    // Select a surface format.
    m_Format.format = VK_FORMAT_B8G8R8_UNORM;
    m_Format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    for (auto const & supportedSurfaceFormat : supportedSurfaceFormats)
    {
        if (supportedSurfaceFormat.format != VK_FORMAT_UNDEFINED)
        {
            m_Format = supportedSurfaceFormat;
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
    swapchainCreateInfo.imageFormat             = m_Format.format;
    swapchainCreateInfo.imageColorSpace         = m_Format.colorSpace;
    swapchainCreateInfo.imageExtent             = m_SurfaceCapabilities.currentExtent;
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

    // Get the images.
    uint32_t imagesCount;
    CheckResult(vkGetSwapchainImagesKHR(m_Renderer->GetDevice(), m_Swapchain, &imagesCount, NULL));
    m_Images.resize(imagesCount);
    CheckResult(vkGetSwapchainImagesKHR(m_Renderer->GetDevice(), m_Swapchain, &imagesCount, m_Images.data()));

    // Create the corresponding image views.
    m_ImageViews.resize(imagesCount);
    for (uint32_t i = 0; i < imagesCount; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType                               = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext                               = NULL;
        imageViewCreateInfo.flags                               = 0;
        imageViewCreateInfo.image                               = m_Images[i];
        imageViewCreateInfo.viewType                            = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format                              = m_Format.format;
        imageViewCreateInfo.components.r                        = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g                        = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b                        = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a                        = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel       = 0;
        imageViewCreateInfo.subresourceRange.levelCount         = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer     = 0;
        imageViewCreateInfo.subresourceRange.layerCount         = 1;
        CheckResult(vkCreateImageView(m_Renderer->GetDevice(), &imageViewCreateInfo, NULL, &m_ImageViews[i]));
    }
}

void Window::Unbind()
{
    for (int i = 0; i < m_ImageViews.size(); ++i)
    {
        vkDestroyImageView(m_Renderer->GetDevice(), m_ImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(m_Renderer->GetDevice(), m_Swapchain, NULL);
    m_Swapchain = VK_NULL_HANDLE;

    vkDestroySurfaceKHR(m_Renderer->GetInstance(), m_Surface, NULL);
    m_Surface = VK_NULL_HANDLE;
}
