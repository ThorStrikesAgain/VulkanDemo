#pragma once

#include <string>

namespace VulkanDemo
{
    class GraphicsHelper;
    class ShaderLoader;
    class VulkanManager;
    class FontManager;

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
        inline GraphicsHelper * GetGraphicsHelper() const { return m_GraphicsHelper; }
        inline FontManager * GetFontManager() const { return m_FontManager; }

        std::string GetExecutablePath() const;

    private:
        Application();

        VulkanManager * m_VulkanManager = nullptr;
        ShaderLoader * m_ShaderLoader = nullptr;
        GraphicsHelper * m_GraphicsHelper = nullptr;
        FontManager * m_FontManager = nullptr;
    };
} // VulkanDemo
