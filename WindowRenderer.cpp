#include "WindowRenderer.h"

#include <cassert>

#include "Application.h"
#include "VulkanManager.h"
#include "Window.h"

namespace VulkanDemo
{
    WindowRenderer::WindowRenderer(Window * window)
    {
        assert(window != nullptr);

        m_VulkanManager = Application::GetInstance().GetVulkanManager();
        m_Window = window;

        CreateRenderPass();
        CreateFramebuffers();
        CreateSynchronization();
        CreateCommandBuffer();
    }

    WindowRenderer::~WindowRenderer()
    {
        WaitForCommandBuffer();

        DestroyCommandBuffer();
        DestroySynchronization();
        DestroyFramebuffers();
        DestroyRenderPass();

        m_Window = nullptr;
        m_VulkanManager = nullptr;
    }

    void WindowRenderer::Render(VkImageView src)
    {
        CheckResult(vkAcquireNextImageKHR(m_VulkanManager->GetDevice(), m_Window->GetSwapchain(), UINT64_MAX, m_ImageAcquiredSemaphore, VK_NULL_HANDLE, &m_NextImageIndex));

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
        renderPassBeginInfo.renderArea.extent = m_Window->GetSurfaceCapabilities().currentExtent;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;

        // Wait and reset the fence if need be, since we only use one command buffer.
        // TODO: Use multiple command buffers.
        WaitForCommandBuffer();

        CheckResult(vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo));

        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (src != VK_NULL_HANDLE)
        {
            // TODO: Copy src to the current framebuffer.
        }

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

        m_CommandBufferPending = true;
        CheckResult(vkQueueSubmit(m_VulkanManager->GetGraphicsQueue(), 1, &submitInfo, m_CommandBufferProcessedFence));

        VkSwapchainKHR swapchain = m_Window->GetSwapchain();
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

    void WindowRenderer::CreateRenderPass()
    {
        // Create the render pass.
        VkAttachmentDescription attachment{};
        attachment.flags = 0;
        attachment.format = m_Window->GetFormat().format;
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

    void WindowRenderer::DestroyRenderPass()
    {
        vkDestroyRenderPass(m_VulkanManager->GetDevice(), m_RenderPass, NULL);
        m_RenderPass = VK_NULL_HANDLE;
    }

    void WindowRenderer::CreateFramebuffers()
    {
        // Create the frame buffers.
        const std::vector<VkImageView> & imageViews = m_Window->GetImageViews();
        const VkSurfaceCapabilitiesKHR & surfaceCapabilities = m_Window->GetSurfaceCapabilities();

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

    void WindowRenderer::DestroyFramebuffers()
    {
        for (int i = 0; i < m_FrameBuffers.size(); ++i)
        {
            vkDestroyFramebuffer(m_VulkanManager->GetDevice(), m_FrameBuffers[i], NULL);
        }
        m_FrameBuffers.clear();
    }

    void WindowRenderer::CreateSynchronization()
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
        fenceCreateInfo.flags = 0;
        CheckResult(vkCreateFence(m_VulkanManager->GetDevice(), &fenceCreateInfo, NULL, &m_CommandBufferProcessedFence));
    }

    void WindowRenderer::DestroySynchronization()
    {
        // TODO: Wait for stuff to become idle.
        vkDestroyFence(m_VulkanManager->GetDevice(), m_CommandBufferProcessedFence, NULL);
        vkDestroySemaphore(m_VulkanManager->GetDevice(), m_ImageRenderedSemaphore, NULL);
        vkDestroySemaphore(m_VulkanManager->GetDevice(), m_ImageAcquiredSemaphore, NULL);
    }

    void WindowRenderer::CreateCommandBuffer()
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = NULL;
        commandBufferAllocateInfo.commandPool = m_VulkanManager->GetGraphicsCommandPool();
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;
        CheckResult(vkAllocateCommandBuffers(m_VulkanManager->GetDevice(), &commandBufferAllocateInfo, &m_CommandBuffer));
    }

    void WindowRenderer::DestroyCommandBuffer()
    {
        vkFreeCommandBuffers(m_VulkanManager->GetDevice(), m_VulkanManager->GetGraphicsCommandPool(), 1, &m_CommandBuffer);
        m_CommandBuffer = VK_NULL_HANDLE;
    }

    void WindowRenderer::WaitForCommandBuffer()
    {
        if (m_CommandBufferPending)
        {
            CheckResult(vkWaitForFences(m_VulkanManager->GetDevice(), 1, &m_CommandBufferProcessedFence, VK_TRUE, UINT64_MAX));
            CheckResult(vkResetFences(m_VulkanManager->GetDevice(), 1, &m_CommandBufferProcessedFence));
            m_CommandBufferPending = false;
        }
    }
} // VulkanDemo
