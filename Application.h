#pragma once

namespace VulkanDemo
{
    class VulkanManager;

    class Application
    {
    public:
        static Application& GetInstance();
        ~Application();

        Application(const Application &) = delete;
        Application & operator=(const Application &) = delete;

        void MainLoop();

        inline VulkanManager * GetVulkanManager() const { return m_VulkanManager; }

    private:
        Application();

        VulkanManager * m_VulkanManager = nullptr;
    };
} // VulkanDemo
