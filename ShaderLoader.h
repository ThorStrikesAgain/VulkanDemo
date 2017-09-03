#pragma once

#include "Shared.h"

#include <vector>

namespace VulkanDemo
{
    class VulkanManager;

    class ShaderLoader
    {
    public:
        ShaderLoader();
        ~ShaderLoader();

        VkShaderModule GetConstVert();
        VkShaderModule GetConstFrag();

        VkShaderModule GetBlitVert();
        VkShaderModule GetBlitFrag();

    private:
        VkShaderModule LoadShaderModule(char const * path);

        std::vector<VkShaderModule> m_Modules;

        VkShaderModule constVertModule = VK_NULL_HANDLE;
        VkShaderModule constFragModule = VK_NULL_HANDLE;

        VkShaderModule blitVertModule = VK_NULL_HANDLE;
        VkShaderModule blitFragModule = VK_NULL_HANDLE;
    };
}
