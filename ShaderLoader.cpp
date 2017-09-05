#include "ShaderLoader.h"

#include <fstream>
#include <limits>
#include <vector>

#include <experimental/filesystem>

#include "Application.h"
#include "VulkanManager.h"

namespace VulkanDemo
{
    ShaderLoader::ShaderLoader()
    {
    }

    ShaderLoader::~ShaderLoader()
    {
        // TODO: Some synchronization mechanism may be required to make sure they aren't used anymore.
        VkDevice device = Application::GetInstance().GetVulkanManager()->GetDevice();
        for (auto shaderModule : m_Modules)
        {
            vkDestroyShaderModule(device, shaderModule, NULL);
        }
        m_Modules.clear();
    }

    // TODO: Move this to a utility class.
    template<typename T>
    std::vector<T> ReadAll(char const * path)
    {
        std::ifstream file;
        file.open(path, std::ios::in | std::ios::binary);
        if (!file.is_open())
        {
            Fail("Failed to open the file.", -1);
        }

        // Get the length.
        file.ignore((std::numeric_limits<std::streamsize>::max)());
        auto length = (size_t)file.gcount();
        if (length % sizeof(T) > 0)
        {
            Fail("The size of the file is not a multiple of the size of the underlying type.", -1);
        }

        // Reset to the beginning.
        file.clear();
        file.seekg(0, std::ios_base::beg);

        // Read everything.
        std::vector<T> content(length / sizeof(T));
        file.read(reinterpret_cast<char *>(content.data()), length);

        return content;
    }

    VkShaderModule ShaderLoader::LoadShaderModule(char const * name)
    {
        VkShaderModule shaderModule = VK_NULL_HANDLE;

        // TODO: This must not be hard-coded like this.
        std::string executableDirectoryPath = Application::GetInstance().GetExecutablePath();
        std::experimental::filesystem::path path =
            std::experimental::filesystem::path(executableDirectoryPath).parent_path().append("spirv").append(name).concat(".spv");

        std::vector<uint32_t> code = ReadAll<uint32_t>(path.string().c_str());
        if (code[0] != 0x07230203)
        {
            // TODO: Support reversed endianness.
            Fail("SPIR-V magic number is incorrect.", -1);
        }

        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.pNext = NULL;
        shaderModuleCreateInfo.flags = 0;
        shaderModuleCreateInfo.codeSize = code.size() * sizeof(uint32_t);
        shaderModuleCreateInfo.pCode = code.data();

        VkDevice device = Application::GetInstance().GetVulkanManager()->GetDevice();
        CheckResult(vkCreateShaderModule(device, &shaderModuleCreateInfo, NULL, &shaderModule));

        m_Modules.push_back(shaderModule);

        return shaderModule;
    }

    VkShaderModule ShaderLoader::GetConstVert()
    {
        if (constVertModule == VK_NULL_HANDLE)
        {
            constVertModule = LoadShaderModule("const.vert");
        }
        return constVertModule;
    }

    VkShaderModule ShaderLoader::GetConstFrag()
    {
        if (constFragModule == VK_NULL_HANDLE)
        {
            constFragModule = LoadShaderModule("const.frag");
        }
        return constFragModule;
    }

    VkShaderModule ShaderLoader::GetBlitVert()
    {
        if (blitVertModule == VK_NULL_HANDLE)
        {
            blitVertModule = LoadShaderModule("blit.vert");
        }
        return blitVertModule;
    }

    VkShaderModule ShaderLoader::GetBlitFrag()
    {
        if (blitFragModule == VK_NULL_HANDLE)
        {
            blitFragModule = LoadShaderModule("blit.frag");
        }
        return blitFragModule;
    }
}
