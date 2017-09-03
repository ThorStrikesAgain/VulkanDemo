#pragma once

#include <string>

namespace VulkanDemo
{
    class VulkanManager;
    class ShaderLoader;

    class Application
    {
    public:
        static Application& GetInstance();
        ~Application();

        Application(const Application &) = delete;
        Application & operator=(const Application &) = delete;

        void MainLoop();

        inline VulkanManager * GetVulkanManager() const { return m_VulkanManager; }
        inline ShaderLoader * GetShaderLoader() const { return m_ShaderLoader; }

        std::string GetExecutablePath() const;

    private:
        Application();

        VulkanManager * m_VulkanManager = nullptr;
        ShaderLoader * m_ShaderLoader = nullptr;
    };
} // VulkanDemo
