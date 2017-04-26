#include "Application.h"

#include "VulkanManager.h"
#include "Window.h"
#include "WindowRenderer.h"

namespace VulkanDemo
{
    Application& Application::GetInstance()
    {
        static Application application;
        return application;
    }

    Application::Application()
    {
        m_VulkanManager = new VulkanManager();
    }

    Application::~Application()
    {
        delete m_VulkanManager;
        m_VulkanManager = nullptr;
    }

    void Application::MainLoop()
    {
        Window w{ 640, 480 };
        WindowRenderer wr{ &w };

        while (w.Run())
        {
            // TODO: Generate a dummy image.
            wr.Render(VK_NULL_HANDLE);
        }
    }

    VulkanManager * Application::GetVulkanManager() const
    {
        return m_VulkanManager;
    }
} // VulkanDemo
