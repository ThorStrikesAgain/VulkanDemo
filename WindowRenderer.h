#pragma once

#include <vector>

#include "Shared.h"

class Renderer;
class Window;

class WindowRenderer
{
public:
    WindowRenderer(Renderer* renderer, Window* window);
    ~WindowRenderer();

    void BeginFrame();
    void EndFrame();

private:
    void CreateRenderPass();
    void DestroyRenderPass();

    void CreateFramebuffers();
    void DestroyFramebuffers();

    void CreateSynchronization();
    void DestroySynchronization();

    void CreateCommandBuffer();
    void DestroyCommandBuffer();

    void WaitForCommandBuffer();

    Renderer*   m_Renderer  = nullptr;
    Window*     m_Window    = nullptr;

    VkRenderPass                m_RenderPass        = VK_NULL_HANDLE;
    std::vector<VkFramebuffer>  m_FrameBuffers;

    uint32_t            m_NextImageIndex            = UINT32_MAX;

    VkSemaphore         m_ImageAcquiredSemaphore        = VK_NULL_HANDLE;
    VkSemaphore         m_ImageRenderedSemaphore        = VK_NULL_HANDLE;
    VkFence             m_CommandBufferProcessedFence   = VK_NULL_HANDLE;
    bool                m_CommandBufferPending          = false;

    VkCommandPool       m_CommandPool               = VK_NULL_HANDLE;
    VkCommandBuffer     m_CommandBuffer             = VK_NULL_HANDLE;
};
