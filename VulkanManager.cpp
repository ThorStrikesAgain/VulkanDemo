#include "VulkanManager.h"

#include <cassert>

#include <Windows.h>
#include <array>
#include <iostream>
#include <sstream>

#include "BUILD_OPTIONS.h"

namespace VulkanDemo
{
    VulkanManager::VulkanManager()
    {
        SetupDebug();
        SetupLayersAndExtensions();

        CreateInstance();
        InitDebug();
        SelectPhysicalDevice();
        CreateDevice();
        CreateCommandPools();
        CreateDescriptorPools();
    }

    VulkanManager::~VulkanManager()
    {
        DestroyDescriptorPools();
        DestroyCommandPools();
        DestroyDevice();
        DeselectPhysicalDevice();
        DeinitDebug();
        DestroyInstance();
    }

    void VulkanManager::CreateInstance()
    {
        DisplayAvailableInstanceLayers();
        DisplayAvailableInstanceExtensions();

        VkApplicationInfo applicationInfo{};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pApplicationName = "VulkanDemo";
        applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.pEngineName = "";
        applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 42);

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if BUILD_ENABLE_VULKAN_DEBUG
        instanceCreateInfo.pNext = &m_DebugReportCallbackCreateInfo;
#endif
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledLayerCount = (uint32_t)m_UsedInstanceLayerNames.size();
        instanceCreateInfo.ppEnabledLayerNames = m_UsedInstanceLayerNames.data();
        instanceCreateInfo.enabledExtensionCount = (uint32_t)m_UsedInstanceExtensionNames.size();
        instanceCreateInfo.ppEnabledExtensionNames = m_UsedInstanceExtensionNames.data();

        CheckResult(vkCreateInstance(&instanceCreateInfo, NULL, &m_Instance));
    }

    void VulkanManager::DestroyInstance()
    {
        vkDestroyInstance(m_Instance, NULL);
        m_Instance = NULL;
    }

    void VulkanManager::SelectPhysicalDevice()
    {
        uint32_t physicalDevicesCount;
        CheckResult(vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, NULL));
        std::vector<VkPhysicalDevice> physicalDevices{ physicalDevicesCount };
        CheckResult(vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, physicalDevices.data()));

        std::cout << "Available Physical Devices:" << std::endl;
        for (auto physicalDevice : physicalDevices)
        {
            VkPhysicalDeviceProperties physicalDeviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
            std::cout << "Device ID: " << physicalDeviceProperties.deviceID << std::endl;
            std::cout << "Device Name: " << physicalDeviceProperties.deviceName << std::endl;
            std::cout << std::endl;
        }

        // Pick the first device.
        m_PhysicalDevice = physicalDevices[0];
    }

    void VulkanManager::DeselectPhysicalDevice()
    {
        m_PhysicalDevice = NULL;
    }

    void VulkanManager::CreateDevice()
    {
        DisplayAvailableDeviceExtensions();

        // Get the queue families.
        uint32_t queueFamiliesCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamiliesCount, NULL);
        std::vector<VkQueueFamilyProperties> queueFamilies{ queueFamiliesCount };
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamiliesCount, queueFamilies.data());

        // Find the first queue suitable for graphics.
        for (int i = 0; i < queueFamilies.size(); ++i)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_GraphicsQueueFamilyIndex = i;
                break;
            }
        }
        assert(m_GraphicsQueueFamilyIndex >= 0 && "Found no queue family with graphics capability.");

        std::array<float, 1> queuePriorities = { 1.0f };
        VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
        graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        graphicsQueueCreateInfo.pNext = NULL;
        graphicsQueueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;
        graphicsQueueCreateInfo.queueCount = (uint32_t)queuePriorities.size();
        graphicsQueueCreateInfo.pQueuePriorities = queuePriorities.data();

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = NULL;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &graphicsQueueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = (uint32_t)m_UsedDeviceExtensionNames.size();
        deviceCreateInfo.ppEnabledExtensionNames = m_UsedDeviceExtensionNames.data();
        deviceCreateInfo.pEnabledFeatures = NULL;

        CheckResult(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, NULL, &m_Device));

        vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
    }

    void VulkanManager::DestroyDevice()
    {
        vkDestroyDevice(m_Device, NULL);
        m_Device = NULL;
    }

    void VulkanManager::CreateCommandPools()
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.pNext = NULL;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;
        CheckResult(vkCreateCommandPool(m_Device, &commandPoolCreateInfo, NULL, &m_GraphicsCommandPool));
    }

    void VulkanManager::DestroyCommandPools()
    {
        vkDestroyCommandPool(m_Device, m_GraphicsCommandPool, NULL);
        m_GraphicsCommandPool = VK_NULL_HANDLE;
    }

    void VulkanManager::CreateDescriptorPools()
    {
        std::array<VkDescriptorPoolSize, 1> poolSizes;

        // TODO: Make this configurable.
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 10;

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = NULL;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCreateInfo.maxSets = 10;
        descriptorPoolCreateInfo.poolSizeCount = (uint32_t)poolSizes.size();
        descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

        CheckResult(vkCreateDescriptorPool(m_Device, &descriptorPoolCreateInfo, NULL, &m_DescriptorPool));
    }

    void VulkanManager::DestroyDescriptorPools()
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, NULL);
        m_DescriptorPool = VK_NULL_HANDLE;
    }

    void VulkanManager::DisplayAvailableInstanceLayers()
    {
        uint32_t layerCount;
        CheckResult(vkEnumerateInstanceLayerProperties(&layerCount, NULL));
        std::vector<VkLayerProperties> availableLayers{ layerCount };
        CheckResult(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

        std::array<char *, 2> headers = { "[Name]", "[Description]" };
        std::cout << "Available Instance Layers:" << std::endl;
        PrintTable((int)headers.size(), layerCount, headers.data(), [availableLayers](int row, int col) {
            if (col == 0) return availableLayers[row].layerName;
            if (col == 1) return availableLayers[row].description;
            return "";
        });
        std::cout << std::endl;
    }

    void VulkanManager::DisplayAvailableInstanceExtensions()
    {
        uint32_t extensionCount;
        CheckResult(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL));
        std::vector<VkExtensionProperties> availableExtensions{ extensionCount };
        CheckResult(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions.data()));

        std::cout << "Available Instance Extensions:" << std::endl;
        PrintTable(1, extensionCount, nullptr, [availableExtensions](int row, int col) {
            return availableExtensions[row].extensionName;
        });
        std::cout << std::endl;
    }

    void VulkanManager::DisplayAvailableDeviceExtensions()
    {
        uint32_t extensionCount;
        CheckResult(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &extensionCount, NULL));
        std::vector<VkExtensionProperties> availableExtensions{ extensionCount };
        CheckResult(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &extensionCount, availableExtensions.data()));

        std::cout << "Available Non-Layer-Specific Device Extensions:" << std::endl;
        PrintTable(1, extensionCount, nullptr, [availableExtensions](int row, int col) {
            return availableExtensions[row].extensionName;
        });
        std::cout << std::endl;
    }

    void VulkanManager::SetupLayersAndExtensions()
    {
        // Instance layers.
        // (none)

        // Instance extensions.
        m_UsedInstanceExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        m_UsedInstanceExtensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

        // Device extensions.
        m_UsedDeviceExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

#if BUILD_ENABLE_VULKAN_DEBUG

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
        VkDebugReportFlagsEXT       flags,
        VkDebugReportObjectTypeEXT  objectType,
        uint64_t                    srcObject,
        size_t                      location,
        int32_t                     msgCode,
        const char *                layerPrefix,
        const char *                msg,
        void *                      userData
    )
    {
        std::ostringstream stream;

        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
            stream << "[INFO] ";
        }
        if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            stream << "[WARNING] ";
        }
        if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            stream << "[PERFORMANCE] ";
        }
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            stream << "[ERROR] ";
        }
        if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            stream << "[DEBUG] ";
        }

        stream << "(@" << layerPrefix << ") " << msg << std::endl;
        std::cout << stream.str();

        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            Fail(stream.str().c_str(), -1);
        }

        return false;
    }

    void VulkanManager::SetupDebug()
    {
        // Parameters
        m_DebugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        m_DebugReportCallbackCreateInfo.pNext = NULL;
        m_DebugReportCallbackCreateInfo.flags =
            //VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
            VK_DEBUG_REPORT_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
            VK_DEBUG_REPORT_ERROR_BIT_EXT |
            //VK_DEBUG_REPORT_DEBUG_BIT_EXT | 
            0;
        m_DebugReportCallbackCreateInfo.pfnCallback = DebugReportCallback;
        m_DebugReportCallbackCreateInfo.pUserData = this;

        // Layers
        m_UsedInstanceLayerNames.push_back("VK_LAYER_LUNARG_standard_validation");

        // Extensions
        m_UsedInstanceExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    void VulkanManager::InitDebug()
    {
        // These commands are not exposed statically and need to be obtained.
        m_CreateDebugReportCallbackCommand = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugReportCallbackEXT");
        assert(m_CreateDebugReportCallbackCommand != 0 && "Failed to retrieve command address.");

        m_DestroyDebugReportCallbackCommand = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugReportCallbackEXT");
        assert(m_DestroyDebugReportCallbackCommand != 0 && "Failed to retrieve command address.");

        CheckResult(m_CreateDebugReportCallbackCommand(m_Instance, &m_DebugReportCallbackCreateInfo, NULL, &m_DebugReportCallback));
    }

    void VulkanManager::DeinitDebug()
    {
        m_DestroyDebugReportCallbackCommand(m_Instance, m_DebugReportCallback, NULL);
        m_DebugReportCallback = VK_NULL_HANDLE;
        m_CreateDebugReportCallbackCommand = NULL;
        m_DestroyDebugReportCallbackCommand = NULL;
    }

#else

    void Renderer::SetupDebug() {}
    void Renderer::InitDebug() {}
    void Renderer::DeinitDebug() {}

#endif // BUILD_ENABLE_VULKAN_DEBUG
} // VulkanDemo
