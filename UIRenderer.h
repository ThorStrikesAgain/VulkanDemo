#pragma once

#include "Shared.h"

namespace VulkanDemo
{
    class Window;
    class VulkanManager;

    // TODO: RENAME UI MANAGER SINCE IT WILL ALSO MANAGE THE MOUSE...
    class UIRenderer
    {
    public:
        UIRenderer(Window * window);
        ~UIRenderer();

        void BeginNewFrame();
        void Draw(VkCommandBuffer commandBuffer);

        bool ProcessEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT * result);

    private:
        Window * m_Window = nullptr;
        VulkanManager * m_VulkanManager = nullptr;
    };
}
