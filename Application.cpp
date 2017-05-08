#include "Application.h"

#include "VulkanManager.h"

#include "Window.h"

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

        while (w.Run())
        {
            w.Render();
        }
    }
} // VulkanDemo
