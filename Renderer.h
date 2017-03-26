#pragma once

#include <vector>

#include "Shared.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

private:
    void CreateInstance();
    void DestroyInstance();

    void DisplayInstanceLayers();
    void DisplayInstanceExtensions();

    VkInstance m_Instance = NULL;

    std::vector<const char*> m_InstanceLayerNames;
    std::vector<const char*> m_InstanceExtensionNames;
};
