#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Renderer
{
public:
    Renderer();
    ~Renderer();

private:
    void CreateInstance();
    void DestroyInstance();

    VkInstance m_Instance = NULL;

    std::vector<const char*> m_InstanceLayerNames;
    std::vector<const char*> m_InstanceExtensionNames;
};
