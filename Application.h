#pragma once

namespace VulkanDemo
{
    class VulkanManager;
    class Window;

    class Application
    {
    public:
        static Application& GetInstance();
        ~Application();

        Application(const Application &) = delete;
        Application & operator=(const Application &) = delete;

        void MainLoop();

        VulkanManager * GetVulkanManager() const;

    private:
        Application();

        VulkanManager * m_VulkanManager = nullptr;
    };
} // VulkanDemo
