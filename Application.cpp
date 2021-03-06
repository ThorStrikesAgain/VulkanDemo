#include "Application.h"

#include "GraphicsHelper.h"
#include "ShaderLoader.h"
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
        m_ShaderLoader = new ShaderLoader();
        m_GraphicsHelper = new GraphicsHelper();
    }

    Application::~Application()
    {
        delete m_GraphicsHelper;
        m_GraphicsHelper = nullptr;

        delete m_ShaderLoader;
        m_ShaderLoader = nullptr;

        delete m_VulkanManager;
        m_VulkanManager = nullptr;
    }

    std::string Application::GetExecutablePath() const
    {
        // TODO: Support more than 260 characters...
        if (_MAX_PATH > 260)
        {
            Fail("Maximum path is potentially too large.");
        }

        // TODO: This doesn't support non-ANSI characters. Use GetModuleFileNameW instead.
        char path[_MAX_PATH];
        DWORD length = GetModuleFileName(NULL, path, _MAX_PATH);
        if (length <= 0)
        {
            Fail("Failed to get the executable directory path.");
        }

        return std::string(path, length);
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
