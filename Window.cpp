#include "Window.h"

#include <cassert>

#include <Windows.h>
#include <array>

#include "Application.h"
#include "BlitPipelineGenerator.h"
#include "GraphicsHelper.h"
#include "ShaderLoader.h"
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
        CreatePipeline();
        CreateFramebuffers();
        CreateSynchronization();
        CreateDescriptorSet();

        m_CommandBufferPool = new CommandBufferPool(m_ImageCount + 1);
    }

    Window::~Window()
    {
        delete m_CommandBufferPool;
        m_CommandBufferPool = nullptr;

        DestroyDescriptorSet();
        DestroySynchronization();
        DestroyFramebuffers();
        DestroyPipeline();
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
            Fail("The selected graphics queue family doesn't support presentation to the Microsoft Windows desktop");
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
            Fail("The selected graphics queue family doesn't support presentation to the specified surface");
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

        // Select a surface format
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
        CheckResult(vkGetSwapchainImagesKHR(m_VulkanManager->GetDevice(), m_Swapchain, &m_ImageCount, NULL));
        m_Images.resize(m_ImageCount);
        CheckResult(vkGetSwapchainImagesKHR(m_VulkanManager->GetDevice(), m_Swapchain, &m_ImageCount, m_Images.data()));

        // Create the corresponding image views.
        m_ImageViews.resize(m_ImageCount);
        for (uint32_t i = 0; i < m_ImageCount; ++i)
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
        for (uint32_t i = 0; i < m_ImageCount; ++i)
        {
            vkDestroyImageView(m_VulkanManager->GetDevice(), m_ImageViews[i], NULL);
        }
        m_ImageViews.clear();

        vkDestroySwapchainKHR(m_VulkanManager->GetDevice(), m_Swapchain, NULL);
        m_Swapchain = VK_NULL_HANDLE;

        vkDestroySurfaceKHR(m_VulkanManager->GetInstance(), m_Surface, NULL);
        m_Surface = VK_NULL_HANDLE;

        m_ImageCount = 0;
    }

    void Window::Render()
    {
        // Render the scene.
        SceneRenderResult renderResult;

        SceneRenderInfo renderInfo{};
        renderInfo.scene = nullptr;
        renderInfo.width = m_Width;
        renderInfo.height = m_Height;
        renderInfo.waitSemaphore = // TODO: We need a texture-usage book keeping mechanism.
            m_BlitCompletedBeforeSceneRendererSemaphoreUsed ? m_BlitCompletedBeforeSceneRenderSemaphore : VK_NULL_HANDLE;
        m_SceneRenderer.Render(renderInfo, renderResult);
        m_SceneRenderedBeforeBlitSemaphore = renderResult.waitSemaphore;

        // Copy to the window.
        CheckResult(vkAcquireNextImageKHR(m_VulkanManager->GetDevice(), m_Swapchain, UINT64_MAX, m_ImageAcquiredBeforeBlitSemaphore, VK_NULL_HANDLE, &m_NextImageIndex));

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = NULL;
        commandBufferBeginInfo.flags = 0;
        commandBufferBeginInfo.pInheritanceInfo = NULL;

        // TODO: Use the right component based on the format.
        VkClearValue clearValue{};
        clearValue.color.float32[0] = 0;
        clearValue.color.float32[1] = 0;
        clearValue.color.float32[2] = 0;
        clearValue.color.float32[3] = 1;

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = NULL;
        renderPassBeginInfo.renderPass = m_RenderPass;
        renderPassBeginInfo.framebuffer = m_FrameBuffers[m_NextImageIndex];
        renderPassBeginInfo.renderArea.extent = m_SurfaceCapabilities.currentExtent;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;

        VkCommandBuffer commandBuffer;
        VkFence commandBufferFence;
        m_CommandBufferPool->GetNextAvailable(&commandBuffer, &commandBufferFence);

        CheckResult(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Blit
        {
            // Bind the pipeline.
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineGenerator->GetPipeline());

            // Update the descriptor set for the blit operation.
            {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = VK_NULL_HANDLE; // We use an immutable sampler.
                imageInfo.imageView = renderResult.imageView;
                imageInfo.imageLayout = renderResult.imageLayout; // Assigned by the scene render constructor.

                VkWriteDescriptorSet writeDescriptorSet{};
                writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSet.pNext = NULL;
                writeDescriptorSet.dstSet = m_BlitDescriptorSet;
                writeDescriptorSet.dstBinding = 0;
                writeDescriptorSet.dstArrayElement = 0;
                writeDescriptorSet.descriptorCount = 1;
                writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptorSet.pImageInfo = &imageInfo;
                writeDescriptorSet.pBufferInfo = NULL;
                writeDescriptorSet.pTexelBufferView = NULL;

                vkUpdateDescriptorSets(m_VulkanManager->GetDevice(), 1, &writeDescriptorSet, 0, NULL);
            }

            // Bind the descriptor set.
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineGenerator->GetPipelineLayout(), 0, 1, &m_BlitDescriptorSet, 0, NULL);

            // Bind the vertex buffer.
            VkBuffer vertices = Application::GetInstance().GetGraphicsHelper()->GetBlitVertices();
            VkDeviceSize offsets = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices, &offsets);

            // Set the viewport.
            VkViewport viewport{};
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = (float)m_Width;
            viewport.height = (float)m_Height;
            viewport.minDepth = 0;
            viewport.maxDepth = 1;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            // Set the scissor.
            VkRect2D scissor{};
            scissor.offset.x = 0;
            scissor.offset.y = 0;
            scissor.extent.width = m_Width;
            scissor.extent.height = m_Height;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            // Draw.
            vkCmdDraw(commandBuffer, 4, 1, 0, 0);
        }

        vkCmdEndRenderPass(commandBuffer);

        CheckResult(vkEndCommandBuffer(commandBuffer));

        std::array<VkSemaphore, 2> semaphoresToWait =
        {
            m_ImageAcquiredBeforeBlitSemaphore,
            m_SceneRenderedBeforeBlitSemaphore
        };

        std::array<VkPipelineStageFlags, 2> semaphoresToWaitMask =
        {
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        };

        std::array<VkSemaphore, 2> semaphoresToSignal =
        {
            m_BlitCompletedBeforePresentSemaphore,
            m_BlitCompletedBeforeSceneRenderSemaphore
        };

        assert(semaphoresToWait.size() == semaphoresToWaitMask.size());

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = NULL;
        submitInfo.waitSemaphoreCount = (uint32_t)semaphoresToWait.size();
        submitInfo.pWaitSemaphores = semaphoresToWait.data();
        submitInfo.pWaitDstStageMask = semaphoresToWaitMask.data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = (uint32_t)semaphoresToSignal.size();
        submitInfo.pSignalSemaphores = semaphoresToSignal.data();

        m_BlitCompletedBeforeSceneRendererSemaphoreUsed = true;

        CheckResult(vkQueueSubmit(m_VulkanManager->GetGraphicsQueue(), 1, &submitInfo, commandBufferFence));

        VkSwapchainKHR swapchain = m_Swapchain;
        VkResult result;
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = NULL;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_BlitCompletedBeforePresentSemaphore;
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

    void Window::CreatePipeline()
    {
        m_PipelineGenerator = new BlitPipelineGenerator(m_RenderPass, 0);
    }

    void Window::DestroyPipeline()
    {
        delete m_PipelineGenerator;
        m_PipelineGenerator = nullptr;
    }

    void Window::CreateFramebuffers()
    {
        // Create the frame buffers.
        const std::vector<VkImageView> & imageViews = m_ImageViews;
        const VkSurfaceCapabilitiesKHR & surfaceCapabilities = m_SurfaceCapabilities;

        m_FrameBuffers.resize(m_ImageCount);
        for (uint32_t i = 0; i < m_ImageCount; ++i)
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
        for (uint32_t i = 0; i < m_ImageCount; ++i)
        {
            vkDestroyFramebuffer(m_VulkanManager->GetDevice(), m_FrameBuffers[i], NULL);
        }
        m_FrameBuffers.clear();
    }

    void Window::CreateSynchronization()
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = NULL;
        semaphoreCreateInfo.flags = 0;

        CheckResult(vkCreateSemaphore(m_VulkanManager->GetDevice(), &semaphoreCreateInfo, NULL, &m_ImageAcquiredBeforeBlitSemaphore));
        CheckResult(vkCreateSemaphore(m_VulkanManager->GetDevice(), &semaphoreCreateInfo, NULL, &m_BlitCompletedBeforePresentSemaphore));
        CheckResult(vkCreateSemaphore(m_VulkanManager->GetDevice(), &semaphoreCreateInfo, NULL, &m_BlitCompletedBeforeSceneRenderSemaphore));
    }

    void Window::DestroySynchronization()
    {
        // TODO: Wait for stuff to become idle.
        VkDevice device = m_VulkanManager->GetDevice();

        vkDestroySemaphore(device, m_BlitCompletedBeforePresentSemaphore, NULL);
        m_BlitCompletedBeforePresentSemaphore = VK_NULL_HANDLE;

        vkDestroySemaphore(device, m_ImageAcquiredBeforeBlitSemaphore, NULL);
        m_ImageAcquiredBeforeBlitSemaphore = VK_NULL_HANDLE;

        vkDestroySemaphore(device, m_BlitCompletedBeforeSceneRenderSemaphore, NULL);
        m_BlitCompletedBeforeSceneRenderSemaphore = VK_NULL_HANDLE;
    }

    void Window::CreateDescriptorSet()
    {
        VkDescriptorSetLayout descriptorSetLayout = m_PipelineGenerator->GetDescriptorSetLayout();

        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.pNext = NULL;
        descriptorSetAllocInfo.descriptorPool = m_VulkanManager->GetDescriptorPool();
        descriptorSetAllocInfo.descriptorSetCount = 1;
        descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;

        CheckResult(vkAllocateDescriptorSets(m_VulkanManager->GetDevice(), &descriptorSetAllocInfo, &m_BlitDescriptorSet));
    }

    void Window::DestroyDescriptorSet()
    {
        CheckResult(vkFreeDescriptorSets(m_VulkanManager->GetDevice(), m_VulkanManager->GetDescriptorPool(), 1, &m_BlitDescriptorSet));
    }

    Window::CommandBufferPool::CommandBufferPool(int size)
    {
        m_Size = size;
        m_CommandBuffers.resize(size);
        m_Fences.resize(size);

        m_VulkanManager = Application::GetInstance().GetVulkanManager();

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = NULL;
        commandBufferAllocateInfo.commandPool = m_VulkanManager->GetGraphicsCommandPool();
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = size;
        CheckResult(vkAllocateCommandBuffers(m_VulkanManager->GetDevice(), &commandBufferAllocateInfo, m_CommandBuffers.data()));
        
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = NULL;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for(int i = 0 ; i < size ; ++i)
        {
            CheckResult(vkCreateFence(m_VulkanManager->GetDevice(), &fenceCreateInfo, NULL, &m_Fences[i]));
        }
    }

    Window::CommandBufferPool::~CommandBufferPool()
    {
        VkDevice device = m_VulkanManager->GetDevice();

        for (int i = 0; i < m_Size; ++i)
        {
            CheckResult(vkWaitForFences(device, 1, &m_Fences[i], VK_TRUE, UINT64_MAX));
            vkDestroyFence(device, m_Fences[i], NULL);
        }
        m_Fences.clear();

        vkFreeCommandBuffers(device, m_VulkanManager->GetGraphicsCommandPool(), m_Size, m_CommandBuffers.data());
        m_CommandBuffers.clear();
    }

    void Window::CommandBufferPool::GetNextAvailable(VkCommandBuffer* commandBuffer, VkFence* fence)
    {
        VkDevice device = m_VulkanManager->GetDevice();

        // Simple LRU.
        CheckResult(vkWaitForFences(device, 1, &m_Fences[m_Current], VK_TRUE, UINT64_MAX));
        CheckResult(vkResetFences(device, 1, &m_Fences[m_Current]));

        *commandBuffer = m_CommandBuffers[m_Current];
        *fence = m_Fences[m_Current];

        m_Current = (m_Current + 1) % m_Size;
    }
} // VulkanDemo
