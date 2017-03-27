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

    PFN_vkCreateDebugReportCallbackEXT      m_CreateDebugReportCallbackCommand      = NULL;
    PFN_vkDestroyDebugReportCallbackEXT     m_DestroyDebugReportCallbackCommand     = NULL;

    // The following structure will be used to capture issues found while creating or destroying an instance (through
    // the pNext member), and also for the persistent callback. See the documentation of the extension for details.
    VkDebugReportCallbackCreateInfoEXT      m_DebugReportCallbackCreateInfo = {};
    VkDebugReportCallbackEXT                m_DebugReportCallback           = VK_NULL_HANDLE;
};
