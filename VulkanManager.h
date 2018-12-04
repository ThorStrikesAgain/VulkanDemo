#pragma once

#include <vector>

#include "Shared.h"

namespace VulkanDemo
{
    class VulkanManager
    {
    public:
        VulkanManager();
        ~VulkanManager();

        inline VkInstance           GetInstance() const { return m_Instance; }
        inline VkPhysicalDevice     GetPhysicalDevice() const { return m_PhysicalDevice; }
        inline VkDevice             GetDevice() const { return m_Device; }

        inline uint32_t             GetGraphicsQueueFamilyIndex() const { return m_GraphicsQueueFamilyIndex; }
        inline VkQueue              GetGraphicsQueue() const{ return m_GraphicsQueue; }

        inline VkCommandPool        GetGraphicsCommandPool() const { return m_GraphicsCommandPool; }
        
        inline VkDescriptorPool     GetDescriptorPool() const { return m_DescriptorPool; }

    private:
        void CreateInstance();
        void DestroyInstance();

        void SelectPhysicalDevice();
        void DeselectPhysicalDevice();

        void CreateDevice();
        void DestroyDevice();

        void CreateCommandPools();
        void DestroyCommandPools();

        void CreateDescriptorPools();
        void DestroyDescriptorPools();

        void DisplayPhysicalDeviceProperties();
        void DisplayAvailableInstanceLayers();
        void DisplayAvailableInstanceExtensions();
        void DisplayAvailableDeviceExtensions();

        void SetupLayersAndExtensions();

        ///
        /// Defines the debugging parameters to be used to initialize the debugging.
        /// Eventually, these should be fetched from a configuration file.
        ///
        void SetupDebug();
        void InitDebug();
        void DeinitDebug();

        VkInstance              m_Instance = NULL;
        VkPhysicalDevice        m_PhysicalDevice = NULL;
        VkDevice                m_Device = NULL;

        std::vector<const char*> m_UsedInstanceLayerNames;
        std::vector<const char*> m_UsedInstanceExtensionNames;
        std::vector<const char*> m_UsedDeviceExtensionNames;

        PFN_vkCreateDebugReportCallbackEXT      m_CreateDebugReportCallbackCommand = NULL;
        PFN_vkDestroyDebugReportCallbackEXT     m_DestroyDebugReportCallbackCommand = NULL;

        // The following structure will be used to capture issues found while creating or destroying an instance (through
        // the pNext member), and also for the persistent callback. See the documentation of the extension for details.
        VkDebugReportCallbackCreateInfoEXT      m_DebugReportCallbackCreateInfo = {};
        VkDebugReportCallbackEXT                m_DebugReportCallback = VK_NULL_HANDLE;

        uint32_t        m_GraphicsQueueFamilyIndex = -1;
        VkQueue         m_GraphicsQueue = NULL;

        // Command Pools
        VkCommandPool   m_GraphicsCommandPool = VK_NULL_HANDLE;

        // Descriptor Pools
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    };
} // VulkanDemo
