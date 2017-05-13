#include "Window.h"

#include <cassert>

#include <Windows.h>
#include <array>

#include "Application.h"
#include "VulkanManager.h"

namespace VulkanDemo
{
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

    Window::Window(int width, int height) : 
        m_SceneRenderer(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        assert(width > 0);
        assert(height > 0);

        m_Width = width;
        m_Height = height;
        m_VulkanManager = Application::GetInstance().GetVulkanManager();

        m_AppInstance = GetModuleHandle(NULL);

        CreateSystemWindow();
        Bind();
        CreateRenderPass();
        CreateFramebuffers();
        CreateSynchronization();
        CreateCommandBuffer();
    }

    Window::~Window()
    {
        WaitForCommandBuffer();
        DestroyCommandBuffer();
        DestroySynchronization();
        DestroyFramebuffers();
        DestroyRenderPass();
        Unbind();
        DestroySystemWindow();

        m_VulkanManager = nullptr;
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

    void Window::CreateSystemWindow()
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

    void Window::DestroySystemWindow()
    {
        DestroyWindow(m_WindowHandle);
        UnregisterClass(m_WindowClassName.c_str(), m_AppInstance);
        m_WindowHandle = NULL;
    }

    void Window::Bind()
    {
        VkPhysicalDevice physicalDevice = m_VulkanManager->GetPhysicalDevice();
        uint32_t queueFamily = m_VulkanManager->GetGraphicsQueueFamilyIndex();

        // Query for Win32 WSI support.
        VkBool32 queueFamilyCanPresentToWindows = vkGetPhysicalDeviceWin32PresentationSupportKHR(
            physicalDevice,
            m_VulkanManager->GetGraphicsQueueFamilyIndex()
        );
        if (!queueFamilyCanPresentToWindows)
        {
            Fail("The selected graphics queue family doesn't support presentation to the Microsoft Windows desktop", -1);
        }

        // Create the surface.
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.pNext = NULL;
        surfaceCreateInfo.hinstance = m_AppInstance;
        surfaceCreateInfo.hwnd = m_WindowHandle;
        CheckResult(vkCreateWin32SurfaceKHR(m_VulkanManager->GetInstance(), &surfaceCreateInfo, NULL, &m_Surface));

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
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.pNext = NULL;
        swapchainCreateInfo.flags = 0;
        swapchainCreateInfo.surface = m_Surface;
        swapchainCreateInfo.minImageCount = selectedImageCount;
        swapchainCreateInfo.imageFormat = m_Format.format;
        swapchainCreateInfo.imageColorSpace = m_Format.colorSpace;
        swapchainCreateInfo.imageExtent = m_SurfaceCapabilities.currentExtent;
        swapchainCreateInfo.imageArrayLayers = 1; // Non-stereoscopic-3D
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 1;
        swapchainCreateInfo.pQueueFamilyIndices = &queueFamily;
        swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = selectedPresentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        CheckResult(vkCreateSwapchainKHR(m_VulkanManager->GetDevice(), &swapchainCreateInfo, NULL, &m_Swapchain));

        // Get the images.
        uint32_t imagesCount;
        CheckResult(vkGetSwapchainImagesKHR(m_VulkanManager->GetDevice(), m_Swapchain, &imagesCount, NULL));
        m_Images.resize(imagesCount);
        CheckResult(vkGetSwapchainImagesKHR(m_VulkanManager->GetDevice(), m_Swapchain, &imagesCount, m_Images.data()));

        // Create the corresponding image views.
        m_ImageViews.resize(imagesCount);
        for (uint32_t i = 0; i < imagesCount; ++i)
        {
            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.pNext = NULL;
            imageViewCreateInfo.flags = 0;
            imageViewCreateInfo.image = m_Images[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = m_Format.format;
            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            CheckResult(vkCreateImageView(m_VulkanManager->GetDevice(), &imageViewCreateInfo, NULL, &m_ImageViews[i]));
        }
    }

    void Window::Unbind()
    {
        for (int i = 0; i < m_ImageViews.size(); ++i)
        {
            vkDestroyImageView(m_VulkanManager->GetDevice(), m_ImageViews[i], NULL);
        }

        vkDestroySwapchainKHR(m_VulkanManager->GetDevice(), m_Swapchain, NULL);
        m_Swapchain = VK_NULL_HANDLE;

        vkDestroySurfaceKHR(m_VulkanManager->GetInstance(), m_Surface, NULL);
        m_Surface = VK_NULL_HANDLE;
    }

    void Window::Render()
    {
        // Render the scene.
        SceneRenderResult renderResult;
        
        SceneRenderInfo renderInfo{};
        renderInfo.scene = nullptr;
        renderInfo.width = m_Width;
        renderInfo.height = m_Height;
        renderInfo.waitSemaphore = m_LastSceneRenderSemaphore;
        m_SceneRenderer.Render(renderInfo, renderResult);
        m_LastSceneRenderSemaphore = renderResult.waitSemaphore;

        // Copy to the window.
        CheckResult(vkAcquireNextImageKHR(m_VulkanManager->GetDevice(), m_Swapchain, UINT64_MAX, m_ImageAcquiredSemaphore, VK_NULL_HANDLE, &m_NextImageIndex));

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = NULL;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        commandBufferBeginInfo.pInheritanceInfo = NULL;

        // TODO: Use the right component based on the format.
        VkClearValue clearValue{};
        clearValue.color.float32[0] = 0;
        clearValue.color.float32[1] = 1.0f;
        clearValue.color.float32[2] = 0;

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = NULL;
        renderPassBeginInfo.renderPass = m_RenderPass;
        renderPassBeginInfo.framebuffer = m_FrameBuffers[m_NextImageIndex];
        renderPassBeginInfo.renderArea.extent = m_SurfaceCapabilities.currentExtent;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;

        // Wait and reset the fence if need be, since we only use one command buffer.
        // TODO: Use multiple command buffers.
        WaitForCommandBuffer();

        CheckResult(vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo));

        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // TODO: Blit...

        vkCmdEndRenderPass(m_CommandBuffer);

        CheckResult(vkEndCommandBuffer(m_CommandBuffer));

        VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = NULL;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &m_ImageAcquiredSemaphore;
        submitInfo.pWaitDstStageMask = &waitDstStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_ImageRenderedSemaphore;

        CheckResult(vkQueueSubmit(m_VulkanManager->GetGraphicsQueue(), 1, &submitInfo, m_CommandBufferProcessedFence));

        VkSwapchainKHR swapchain = m_Swapchain;
        VkResult result;
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = NULL;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_ImageRenderedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &m_NextImageIndex;
        presentInfo.pResults = &result;

        CheckResult(vkQueuePresentKHR(m_VulkanManager->GetGraphicsQueue(), &presentInfo));
        CheckResult(result);
    }

    void Window::CreateRenderPass()
    {
        // Create the render pass.
        VkAttachmentDescription attachment{};
        attachment.flags = 0;
        attachment.format = m_Format.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference attachmentReference{};
        attachmentReference.attachment = 0;
        attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.flags = 0;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = NULL;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attachmentReference;
        subpass.pResolveAttachments = NULL;
        subpass.pDepthStencilAttachment = NULL;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = NULL;

        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.pNext = NULL;
        renderPassCreateInfo.flags = 0;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &attachment;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = 0;
        renderPassCreateInfo.pDependencies = NULL;
        CheckResult(vkCreateRenderPass(m_VulkanManager->GetDevice(), &renderPassCreateInfo, NULL, &m_RenderPass));
    }

    void Window::DestroyRenderPass()
    {
        vkDestroyRenderPass(m_VulkanManager->GetDevice(), m_RenderPass, NULL);
        m_RenderPass = VK_NULL_HANDLE;
    }

    void Window::CreateFramebuffers()
    {
        // Create the frame buffers.
        const std::vector<VkImageView> & imageViews = m_ImageViews;
        const VkSurfaceCapabilitiesKHR & surfaceCapabilities = m_SurfaceCapabilities;

        m_FrameBuffers.resize(imageViews.size());
        for (int i = 0; i < m_FrameBuffers.size(); ++i)
        {
            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.pNext = NULL;
            framebufferCreateInfo.flags = 0;
            framebufferCreateInfo.renderPass = m_RenderPass;
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.pAttachments = &imageViews[i];
            framebufferCreateInfo.width = surfaceCapabilities.currentExtent.width;
            framebufferCreateInfo.height = surfaceCapabilities.currentExtent.height;
            framebufferCreateInfo.layers = 1;
            CheckResult(vkCreateFramebuffer(m_VulkanManager->GetDevice(), &framebufferCreateInfo, NULL, &m_FrameBuffers[i]));
        }
    }

    void Window::DestroyFramebuffers()
    {
        for (int i = 0; i < m_FrameBuffers.size(); ++i)
        {
            vkDestroyFramebuffer(m_VulkanManager->GetDevice(), m_FrameBuffers[i], NULL);
        }
        m_FrameBuffers.clear();
    }

    void Window::CreateSynchronization()
    {
        // Create a semaphore used to ensure that the command buffer waits for the image to actually be acquired.
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = NULL;
        semaphoreCreateInfo.flags = 0;
        CheckResult(vkCreateSemaphore(m_VulkanManager->GetDevice(), &semaphoreCreateInfo, NULL, &m_ImageAcquiredSemaphore));
        CheckResult(vkCreateSemaphore(m_VulkanManager->GetDevice(), &semaphoreCreateInfo, NULL, &m_ImageRenderedSemaphore));

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = NULL;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        CheckResult(vkCreateFence(m_VulkanManager->GetDevice(), &fenceCreateInfo, NULL, &m_CommandBufferProcessedFence));
    }

    void Window::DestroySynchronization()
    {
        // TODO: Wait for stuff to become idle.
        vkDestroyFence(m_VulkanManager->GetDevice(), m_CommandBufferProcessedFence, NULL);
        vkDestroySemaphore(m_VulkanManager->GetDevice(), m_ImageRenderedSemaphore, NULL);
        vkDestroySemaphore(m_VulkanManager->GetDevice(), m_ImageAcquiredSemaphore, NULL);
    }

    void Window::CreateCommandBuffer()
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = NULL;
        commandBufferAllocateInfo.commandPool = m_VulkanManager->GetGraphicsCommandPool();
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;
        CheckResult(vkAllocateCommandBuffers(m_VulkanManager->GetDevice(), &commandBufferAllocateInfo, &m_CommandBuffer));
    }

    void Window::DestroyCommandBuffer()
    {
        vkFreeCommandBuffers(m_VulkanManager->GetDevice(), m_VulkanManager->GetGraphicsCommandPool(), 1, &m_CommandBuffer);
        m_CommandBuffer = VK_NULL_HANDLE;
    }

    void Window::WaitForCommandBuffer()
    {
        CheckResult(vkWaitForFences(m_VulkanManager->GetDevice(), 1, &m_CommandBufferProcessedFence, VK_TRUE, UINT64_MAX));
        CheckResult(vkResetFences(m_VulkanManager->GetDevice(), 1, &m_CommandBufferProcessedFence));
    }
} // VulkanDemo
