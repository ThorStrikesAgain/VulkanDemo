#include "SceneRenderer.h"

#include <array>
#include <cassert>

#include "Application.h"
#include "Configuration.h"
#include "Scene.h"
#include "VulkanManager.h"

namespace VulkanDemo
{
    SceneRenderer::SceneRenderer(VkImageLayout colorLayout) :
        m_ColorLayout(colorLayout)
    {
        m_VulkanManager = Application::GetInstance().GetVulkanManager();

        CreateForwardRenderPass();
        CreateCommandBuffer();
    }

    SceneRenderer::~SceneRenderer()
    {
        if (m_IsInitialized)
        {
            DestroyFramebuffer();
        }

        DestroyCommandBuffer();
        DestroyForwardRenderpass();

        m_VulkanManager = nullptr;
    }

    VkImage SceneRenderer::Render(const Scene * scene, int width, int height)
    {
        UpdateFramebuffer(width, height);
        // TODO: Render the objects according to the camera.

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = NULL;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        commandBufferBeginInfo.pInheritanceInfo = NULL;
        CheckResult(vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo));
        
        VkClearValue clearValue{};
        clearValue.color.float32[0] = 0.0f;
        clearValue.color.float32[1] = 0.0f;
        clearValue.color.float32[2] = 1.0f;
        clearValue.color.float32[3] = 1.0f;
        clearValue.depthStencil.depth = 1.0f;
        clearValue.depthStencil.stencil = 0;

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = NULL;
        renderPassBeginInfo.renderPass = m_ForwardRenderPass;
        renderPassBeginInfo.framebuffer = m_ForwardFramebuffer;
        renderPassBeginInfo.renderArea.extent.width = m_Width;
        renderPassBeginInfo.renderArea.extent.height = m_Height;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = &clearValue;
        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdEndRenderPass(m_CommandBuffer);

        CheckResult(vkEndCommandBuffer(m_CommandBuffer));

        // For now, we'll just clear an empty image with the right size.
        return VK_NULL_HANDLE;
    }

    void SceneRenderer::CreateForwardRenderPass()
    {
        std::array<VkAttachmentDescription, 2> attachments;

        // Color attachment used for forward rendering, and returned.
        attachments[0].flags = 0;
        attachments[0].format = Configuration::ForwardRendererColorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = m_ColorLayout;

        // Depth attachment used for forward rendering.
        attachments[1].flags = 0;
        attachments[1].format = Configuration::ForwardRendererDepthStencilFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentReference{};
        depthAttachmentReference.attachment = 1;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription forwardSubpass{};
        forwardSubpass.flags = 0;
        forwardSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        forwardSubpass.inputAttachmentCount = 0;
        forwardSubpass.pInputAttachments = NULL;
        forwardSubpass.colorAttachmentCount = 1;
        forwardSubpass.pColorAttachments = &colorAttachmentReference;
        forwardSubpass.pResolveAttachments = NULL;
        forwardSubpass.pDepthStencilAttachment = &depthAttachmentReference;
        forwardSubpass.preserveAttachmentCount = 0;
        forwardSubpass.pPreserveAttachments = NULL;

        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.pNext = NULL;
        renderPassCreateInfo.flags = 0;
        renderPassCreateInfo.attachmentCount = 2;
        renderPassCreateInfo.pAttachments = attachments.data();
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &forwardSubpass;
        renderPassCreateInfo.dependencyCount = 0;
        renderPassCreateInfo.pDependencies = NULL;

        CheckResult(vkCreateRenderPass(m_VulkanManager->GetDevice(), &renderPassCreateInfo, NULL, &m_ForwardRenderPass));
    }

    void SceneRenderer::DestroyForwardRenderpass()
    {
        vkDestroyRenderPass(m_VulkanManager->GetDevice(), m_ForwardRenderPass, NULL);
    }

    void SceneRenderer::UpdateFramebuffer(int width, int height)
    {
        if (m_IsInitialized)
        {
            if (m_Width == width && m_Height == height)
            {
                return;
            }
            DestroyFramebuffer();
        }

        CreateFramebuffer(width, height);
    }

    void SceneRenderer::CreateFramebuffer(int width, int height)
    {
        assert(!m_IsInitialized);

        VkDevice device = m_VulkanManager->GetDevice();

        m_Width = width;
        m_Height = height;

        m_IsInitialized = true;

        uint32_t graphicsQueueFamilyIndex = m_VulkanManager->GetGraphicsQueueFamilyIndex();

        // Color
        {
            VkImageCreateInfo colorImageCreateInfo{};
            colorImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            colorImageCreateInfo.pNext = NULL;
            colorImageCreateInfo.flags = 0;
            colorImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            colorImageCreateInfo.format = Configuration::ForwardRendererColorFormat;
            colorImageCreateInfo.extent.width = width;
            colorImageCreateInfo.extent.height = height;
            colorImageCreateInfo.extent.depth = 1;
            colorImageCreateInfo.mipLevels = 1;
            colorImageCreateInfo.arrayLayers = 1;
            colorImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            colorImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            colorImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            colorImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            colorImageCreateInfo.queueFamilyIndexCount = 1;
            colorImageCreateInfo.pQueueFamilyIndices = &graphicsQueueFamilyIndex;
            colorImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            CheckResult(vkCreateImage(m_VulkanManager->GetDevice(), &colorImageCreateInfo, NULL, &m_ForwardColorImage));
            m_ForwardColorMemory = AllocateAndBindImageMemory(m_ForwardColorImage);

            VkImageViewCreateInfo colorImageViewCreateInfo{};
            colorImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorImageViewCreateInfo.pNext = NULL;
            colorImageViewCreateInfo.flags = 0;
            colorImageViewCreateInfo.image = m_ForwardColorImage;
            colorImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            colorImageViewCreateInfo.format = Configuration::ForwardRendererColorFormat;
            colorImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            colorImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            colorImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            colorImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            colorImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            colorImageViewCreateInfo.subresourceRange.levelCount = 1;
            colorImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            colorImageViewCreateInfo.subresourceRange.layerCount = 1;
            CheckResult(vkCreateImageView(m_VulkanManager->GetDevice(), &colorImageViewCreateInfo, NULL, &m_ForwardColorImageView));
        }

        // Depth-Stencil
        {
            VkImageCreateInfo depthStencilImageCreateInfo{};
            depthStencilImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            depthStencilImageCreateInfo.pNext = NULL;
            depthStencilImageCreateInfo.flags = 0;
            depthStencilImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            depthStencilImageCreateInfo.format = Configuration::ForwardRendererDepthStencilFormat;
            depthStencilImageCreateInfo.extent.width = width;
            depthStencilImageCreateInfo.extent.height = height;
            depthStencilImageCreateInfo.extent.depth = 1;
            depthStencilImageCreateInfo.mipLevels = 1;
            depthStencilImageCreateInfo.arrayLayers = 1;
            depthStencilImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            depthStencilImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            depthStencilImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depthStencilImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            depthStencilImageCreateInfo.queueFamilyIndexCount = 1;
            depthStencilImageCreateInfo.pQueueFamilyIndices = &graphicsQueueFamilyIndex;;
            depthStencilImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            CheckResult(vkCreateImage(m_VulkanManager->GetDevice(), &depthStencilImageCreateInfo, NULL, &m_ForwardDepthStencilImage));
            m_ForwardDepthStencilMemory = AllocateAndBindImageMemory(m_ForwardDepthStencilImage);

            VkImageViewCreateInfo depthStencilImageViewCreateInfo{};
            depthStencilImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            depthStencilImageViewCreateInfo.pNext = NULL;
            depthStencilImageViewCreateInfo.flags = 0;
            depthStencilImageViewCreateInfo.image = m_ForwardDepthStencilImage;
            depthStencilImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depthStencilImageViewCreateInfo.format = Configuration::ForwardRendererDepthStencilFormat;
            depthStencilImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            depthStencilImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            depthStencilImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            depthStencilImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            depthStencilImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            depthStencilImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            depthStencilImageViewCreateInfo.subresourceRange.levelCount = 1;
            depthStencilImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            depthStencilImageViewCreateInfo.subresourceRange.layerCount = 1;
            CheckResult(vkCreateImageView(m_VulkanManager->GetDevice(), &depthStencilImageViewCreateInfo, NULL, &m_ForwardDepthStencilImageView));
        }

        // Framebuffer
        {
            std::array<VkImageView, 2> attachments{ m_ForwardColorImageView, m_ForwardDepthStencilImageView };

            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.pNext = NULL;
            framebufferCreateInfo.flags = 0;
            framebufferCreateInfo.renderPass = m_ForwardRenderPass;
            framebufferCreateInfo.attachmentCount = (uint32_t)attachments.size();
            framebufferCreateInfo.pAttachments = attachments.data();
            framebufferCreateInfo.width = m_Width;
            framebufferCreateInfo.height = m_Height;
            framebufferCreateInfo.layers = 1;
            CheckResult(vkCreateFramebuffer(m_VulkanManager->GetDevice(), &framebufferCreateInfo, NULL, &m_ForwardFramebuffer));
        }
    }

    void SceneRenderer::DestroyFramebuffer()
    {
        assert(m_IsInitialized);

        vkDestroyFramebuffer(m_VulkanManager->GetDevice(), m_ForwardFramebuffer, NULL);

        vkDestroyImageView(m_VulkanManager->GetDevice(), m_ForwardDepthStencilImageView, NULL);
        vkFreeMemory(m_VulkanManager->GetDevice(), m_ForwardDepthStencilMemory, NULL);
        vkDestroyImage(m_VulkanManager->GetDevice(), m_ForwardDepthStencilImage, NULL);
        
        vkDestroyImageView(m_VulkanManager->GetDevice(), m_ForwardColorImageView, NULL);
        vkFreeMemory(m_VulkanManager->GetDevice(), m_ForwardColorMemory, NULL);
        vkDestroyImage(m_VulkanManager->GetDevice(), m_ForwardColorImage, NULL);

        m_IsInitialized = false;
    }

    void SceneRenderer::CreateCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.pNext = NULL;
        allocateInfo.commandPool = m_VulkanManager->GetGraphicsCommandPool();
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;
        CheckResult(vkAllocateCommandBuffers(m_VulkanManager->GetDevice(), &allocateInfo, &m_CommandBuffer));
    }

    void SceneRenderer::DestroyCommandBuffer()
    {
        vkFreeCommandBuffers(m_VulkanManager->GetDevice(), m_VulkanManager->GetGraphicsCommandPool(), 1, &m_CommandBuffer);
        m_CommandBuffer = VK_NULL_HANDLE;
    }
} // VulkanDemo