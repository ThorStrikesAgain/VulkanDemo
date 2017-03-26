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
    
    void SetupLayersAndExtensions();

    ///
    /// Defines the debugging parameters to be used to initialize the debugging.
    /// Eventually, these should be fetched from a configuration file.
    ///
    void SetupDebug();
    void InitDebug();
    void DeinitDebug();
    
    VkInstance m_Instance                                                       = NULL;

    std::vector<const char*> m_UsedInstanceLayerNames;
    std::vector<const char*> m_UsedInstanceExtensionNames;

    PFN_vkCreateDebugReportCallbackEXT m_CreateDebugReportCallbackCommand       = NULL;
    PFN_vkDestroyDebugReportCallbackEXT m_DestroyDebugReportCallbackCommand     = NULL;

    VkDebugReportFlagsEXT m_DebugReportFlags                                    = 0;
    VkDebugReportCallbackEXT m_DebugReportCallback                              = VK_NULL_HANDLE;
};
