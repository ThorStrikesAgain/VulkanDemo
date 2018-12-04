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

        DisplayPhysicalDeviceProperties();
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

    void VulkanManager::DisplayPhysicalDeviceProperties()
    {
        std::cout << "Physical Device Properties:" << std::endl;
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        std::cout << "apiVersion = " << properties.apiVersion << std::endl;
        std::cout << "driverVersion = " << properties.driverVersion << std::endl;
        std::cout << "vendorID = " << properties.vendorID << std::endl;
        std::cout << "deviceID = " << properties.deviceID << std::endl;
        std::cout << "deviceType = ";
        switch (properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_OTHER";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_CPU";
                break;
        }
        std::cout << std::endl;
        std::cout << "deviceName = " << properties.deviceName << std::endl;
        std::cout << "limits =" << std::endl;
        std::cout << "\t" << "maxImageDimension1D = " << properties.limits.maxImageDimension1D << std::endl;
        std::cout << "\t" << "maxImageDimension2D = " << properties.limits.maxImageDimension2D << std::endl;
        std::cout << "\t" << "maxImageDimension3D = " << properties.limits.maxImageDimension3D << std::endl;
        std::cout << "\t" << "maxImageDimensionCube = " << properties.limits.maxImageDimensionCube << std::endl;
        std::cout << "\t" << "maxImageArrayLayers = " << properties.limits.maxImageArrayLayers << std::endl;
        std::cout << "\t" << "maxTexelBufferElements = " << properties.limits.maxTexelBufferElements << std::endl;
        std::cout << "\t" << "maxUniformBufferRange = " << properties.limits.maxUniformBufferRange << std::endl;
        std::cout << "\t" << "maxStorageBufferRange = " << properties.limits.maxStorageBufferRange << std::endl;
        std::cout << "\t" << "maxPushConstantsSize = " << properties.limits.maxPushConstantsSize << std::endl;
        std::cout << "\t" << "maxMemoryAllocationCount = " << properties.limits.maxMemoryAllocationCount << std::endl;
        std::cout << "\t" << "maxSamplerAllocationCount = " << properties.limits.maxSamplerAllocationCount << std::endl;
        std::cout << "\t" << "bufferImageGranularity = " << properties.limits.bufferImageGranularity << std::endl;
        std::cout << "\t" << "sparseAddressSpaceSize = " << properties.limits.sparseAddressSpaceSize << std::endl;
        std::cout << "\t" << "maxBoundDescriptorSets = " << properties.limits.maxBoundDescriptorSets << std::endl;
        std::cout << "\t" << "maxPerStageDescriptorSamplers = " << properties.limits.maxPerStageDescriptorSamplers << std::endl;
        std::cout << "\t" << "maxPerStageDescriptorUniformBuffers = " << properties.limits.maxPerStageDescriptorUniformBuffers << std::endl;
        std::cout << "\t" << "maxPerStageDescriptorStorageBuffers = " << properties.limits.maxPerStageDescriptorStorageBuffers << std::endl;
        std::cout << "\t" << "maxPerStageDescriptorSampledImages = " << properties.limits.maxPerStageDescriptorSampledImages << std::endl;
        std::cout << "\t" << "maxPerStageDescriptorStorageImages = " << properties.limits.maxPerStageDescriptorStorageImages << std::endl;
        std::cout << "\t" << "maxPerStageDescriptorInputAttachments = " << properties.limits.maxPerStageDescriptorInputAttachments << std::endl;
        std::cout << "\t" << "maxPerStageResources = " << properties.limits.maxPerStageResources << std::endl;
        std::cout << "\t" << "maxDescriptorSetSamplers = " << properties.limits.maxDescriptorSetSamplers << std::endl;
        std::cout << "\t" << "maxDescriptorSetUniformBuffers = " << properties.limits.maxDescriptorSetUniformBuffers << std::endl;
        std::cout << "\t" << "maxDescriptorSetUniformBuffersDynamic = " << properties.limits.maxDescriptorSetUniformBuffersDynamic << std::endl;
        std::cout << "\t" << "maxDescriptorSetStorageBuffers = " << properties.limits.maxDescriptorSetStorageBuffers << std::endl;
        std::cout << "\t" << "maxDescriptorSetStorageBuffersDynamic = " << properties.limits.maxDescriptorSetStorageBuffersDynamic << std::endl;
        std::cout << "\t" << "maxDescriptorSetSampledImages = " << properties.limits.maxDescriptorSetSampledImages << std::endl;
        std::cout << "\t" << "maxDescriptorSetStorageImages = " << properties.limits.maxDescriptorSetStorageImages << std::endl;
        std::cout << "\t" << "maxDescriptorSetInputAttachments = " << properties.limits.maxDescriptorSetInputAttachments << std::endl;
        std::cout << "\t" << "maxVertexInputAttributes = " << properties.limits.maxVertexInputAttributes << std::endl;
        std::cout << "\t" << "maxVertexInputBindings = " << properties.limits.maxVertexInputBindings << std::endl;
        std::cout << "\t" << "maxVertexInputAttributeOffset = " << properties.limits.maxVertexInputAttributeOffset << std::endl;
        std::cout << "\t" << "maxVertexInputBindingStride = " << properties.limits.maxVertexInputBindingStride << std::endl;
        std::cout << "\t" << "maxVertexOutputComponents = " << properties.limits.maxVertexOutputComponents << std::endl;
        std::cout << "\t" << "maxTessellationGenerationLevel = " << properties.limits.maxTessellationGenerationLevel << std::endl;
        std::cout << "\t" << "maxTessellationPatchSize = " << properties.limits.maxTessellationPatchSize << std::endl;
        std::cout << "\t" << "maxTessellationControlPerVertexInputComponents = " << properties.limits.maxTessellationControlPerVertexInputComponents << std::endl;
        std::cout << "\t" << "maxTessellationControlPerVertexOutputComponents = " << properties.limits.maxTessellationControlPerVertexOutputComponents << std::endl;
        std::cout << "\t" << "maxTessellationControlPerPatchOutputComponents = " << properties.limits.maxTessellationControlPerPatchOutputComponents << std::endl;
        std::cout << "\t" << "maxTessellationControlTotalOutputComponents = " << properties.limits.maxTessellationControlTotalOutputComponents << std::endl;
        std::cout << "\t" << "maxTessellationEvaluationInputComponents = " << properties.limits.maxTessellationEvaluationInputComponents << std::endl;
        std::cout << "\t" << "maxTessellationEvaluationOutputComponents = " << properties.limits.maxTessellationEvaluationOutputComponents << std::endl;
        std::cout << "\t" << "maxGeometryShaderInvocations = " << properties.limits.maxGeometryShaderInvocations << std::endl;
        std::cout << "\t" << "maxGeometryInputComponents = " << properties.limits.maxGeometryInputComponents << std::endl;
        std::cout << "\t" << "maxGeometryOutputComponents = " << properties.limits.maxGeometryOutputComponents << std::endl;
        std::cout << "\t" << "maxGeometryOutputVertices = " << properties.limits.maxGeometryOutputVertices << std::endl;
        std::cout << "\t" << "maxGeometryTotalOutputComponents = " << properties.limits.maxGeometryTotalOutputComponents << std::endl;
        std::cout << "\t" << "maxFragmentInputComponents = " << properties.limits.maxFragmentInputComponents << std::endl;
        std::cout << "\t" << "maxFragmentOutputAttachments = " << properties.limits.maxFragmentOutputAttachments << std::endl;
        std::cout << "\t" << "maxFragmentDualSrcAttachments = " << properties.limits.maxFragmentDualSrcAttachments << std::endl;
        std::cout << "\t" << "maxFragmentCombinedOutputResources = " << properties.limits.maxFragmentCombinedOutputResources << std::endl;
        std::cout << "\t" << "maxComputeSharedMemorySize = " << properties.limits.maxComputeSharedMemorySize << std::endl;
        std::cout << "\t" << "maxComputeWorkGroupCount = " << 
            properties.limits.maxComputeWorkGroupCount[0] << " " <<
            properties.limits.maxComputeWorkGroupCount[1] << " " <<
            properties.limits.maxComputeWorkGroupCount[2] << std::endl;
        std::cout << "\t" << "maxComputeWorkGroupInvocations = " << properties.limits.maxComputeWorkGroupInvocations << std::endl;
        std::cout << "\t" << "maxComputeWorkGroupSize = " <<
            properties.limits.maxComputeWorkGroupSize[0] << " " <<
            properties.limits.maxComputeWorkGroupSize[1] << " " <<
            properties.limits.maxComputeWorkGroupSize[2] << std::endl;
        std::cout << "\t" << "subPixelPrecisionBits = " << properties.limits.subPixelPrecisionBits << std::endl;
        std::cout << "\t" << "subTexelPrecisionBits = " << properties.limits.subTexelPrecisionBits << std::endl;
        std::cout << "\t" << "mipmapPrecisionBits = " << properties.limits.mipmapPrecisionBits << std::endl;
        std::cout << "\t" << "maxDrawIndexedIndexValue = " << properties.limits.maxDrawIndexedIndexValue << std::endl;
        std::cout << "\t" << "maxDrawIndirectCount = " << properties.limits.maxDrawIndirectCount << std::endl;
        std::cout << "\t" << "maxSamplerLodBias = " << properties.limits.maxSamplerLodBias << std::endl;
        std::cout << "\t" << "maxSamplerAnisotropy = " << properties.limits.maxSamplerAnisotropy << std::endl;
        std::cout << "\t" << "maxViewports = " << properties.limits.maxViewports << std::endl;
        std::cout << "\t" << "maxViewportDimensions = " << 
            properties.limits.maxViewportDimensions[0] << " " << 
            properties.limits.maxViewportDimensions[1] << std::endl;
        std::cout << "\t" << "viewportBoundsRange = " << 
            properties.limits.viewportBoundsRange[0] << " " <<
            properties.limits.viewportBoundsRange[1] << std::endl;
        std::cout << "\t" << "viewportSubPixelBits = " << properties.limits.viewportSubPixelBits << std::endl;
        std::cout << "\t" << "minMemoryMapAlignment = " << properties.limits.minMemoryMapAlignment << std::endl;
        std::cout << "\t" << "minTexelBufferOffsetAlignment = " << properties.limits.minTexelBufferOffsetAlignment << std::endl;
        std::cout << "\t" << "minUniformBufferOffsetAlignment = " << properties.limits.minUniformBufferOffsetAlignment << std::endl;
        std::cout << "\t" << "minStorageBufferOffsetAlignment = " << properties.limits.minStorageBufferOffsetAlignment << std::endl;
        std::cout << "\t" << "minTexelOffset = " << properties.limits.minTexelOffset << std::endl;
        std::cout << "\t" << "maxTexelOffset = " << properties.limits.maxTexelOffset << std::endl;
        std::cout << "\t" << "minTexelGatherOffset = " << properties.limits.minTexelGatherOffset << std::endl;
        std::cout << "\t" << "maxTexelGatherOffset = " << properties.limits.maxTexelGatherOffset << std::endl;
        std::cout << "\t" << "minInterpolationOffset = " << properties.limits.minInterpolationOffset << std::endl;
        std::cout << "\t" << "maxInterpolationOffset = " << properties.limits.maxInterpolationOffset << std::endl;
        std::cout << "\t" << "subPixelInterpolationOffsetBits = " << properties.limits.subPixelInterpolationOffsetBits << std::endl;
        std::cout << "\t" << "maxFramebufferWidth = " << properties.limits.maxFramebufferWidth << std::endl;
        std::cout << "\t" << "maxFramebufferHeight = " << properties.limits.maxFramebufferHeight << std::endl;
        std::cout << "\t" << "maxFramebufferLayers = " << properties.limits.maxFramebufferLayers << std::endl;
        std::cout << "\t" << "framebufferColorSampleCounts = " << properties.limits.framebufferColorSampleCounts << std::endl;
        std::cout << "\t" << "framebufferDepthSampleCounts = " << properties.limits.framebufferDepthSampleCounts << std::endl;
        std::cout << "\t" << "framebufferStencilSampleCounts = " << properties.limits.framebufferStencilSampleCounts << std::endl;
        std::cout << "\t" << "framebufferNoAttachmentsSampleCounts = " << properties.limits.framebufferNoAttachmentsSampleCounts << std::endl;
        std::cout << "\t" << "maxColorAttachments = " << properties.limits.maxColorAttachments << std::endl;
        std::cout << "\t" << "sampledImageColorSampleCounts = " << properties.limits.sampledImageColorSampleCounts << std::endl;
        std::cout << "\t" << "sampledImageIntegerSampleCounts = " << properties.limits.sampledImageIntegerSampleCounts << std::endl;
        std::cout << "\t" << "sampledImageDepthSampleCounts = " << properties.limits.sampledImageDepthSampleCounts << std::endl;
        std::cout << "\t" << "sampledImageStencilSampleCounts = " << properties.limits.sampledImageStencilSampleCounts << std::endl;
        std::cout << "\t" << "storageImageSampleCounts = " << properties.limits.storageImageSampleCounts << std::endl;
        std::cout << "\t" << "maxSampleMaskWords = " << properties.limits.maxSampleMaskWords << std::endl;
        std::cout << "\t" << "timestampComputeAndGraphics = " << properties.limits.timestampComputeAndGraphics << std::endl;
        std::cout << "\t" << "timestampPeriod = " << properties.limits.timestampPeriod << std::endl;
        std::cout << "\t" << "maxClipDistances = " << properties.limits.maxClipDistances << std::endl;
        std::cout << "\t" << "maxCullDistances = " << properties.limits.maxCullDistances << std::endl;
        std::cout << "\t" << "maxCombinedClipAndCullDistances = " << properties.limits.maxCombinedClipAndCullDistances << std::endl;
        std::cout << "\t" << "discreteQueuePriorities = " << properties.limits.discreteQueuePriorities << std::endl;
        std::cout << "\t" << "pointSizeRange = " << 
            properties.limits.pointSizeRange[0] << " " <<
            properties.limits.pointSizeRange[1] << std::endl;
        std::cout << "\t" << "lineWidthRange = " <<
            properties.limits.lineWidthRange[0] << " " <<
            properties.limits.lineWidthRange[1] << std::endl;
        std::cout << "\t" << "pointSizeGranularity = " << properties.limits.pointSizeGranularity << std::endl;
        std::cout << "\t" << "lineWidthGranularity = " << properties.limits.lineWidthGranularity << std::endl;
        std::cout << "\t" << "strictLines = " << properties.limits.strictLines << std::endl;
        std::cout << "\t" << "standardSampleLocations = " << properties.limits.standardSampleLocations << std::endl;
        std::cout << "\t" << "optimalBufferCopyOffsetAlignment = " << properties.limits.optimalBufferCopyOffsetAlignment << std::endl;
        std::cout << "\t" << "optimalBufferCopyRowPitchAlignment = " << properties.limits.optimalBufferCopyRowPitchAlignment << std::endl;
        std::cout << "\t" << "nonCoherentAtomSize = " << properties.limits.nonCoherentAtomSize << std::endl;
        std::cout << "sparseProperties =" << std::endl;
        std::cout << "\t" << "residencyStandard2DBlockShape = " << properties.sparseProperties.residencyStandard2DBlockShape << std::endl;
        std::cout << "\t" << "residencyStandard2DMultisampleBlockShape = " << properties.sparseProperties.residencyStandard2DMultisampleBlockShape << std::endl;
        std::cout << "\t" << "residencyStandard3DBlockShape = " << properties.sparseProperties.residencyStandard3DBlockShape << std::endl;
        std::cout << "\t" << "residencyAlignedMipSize = " << properties.sparseProperties.residencyAlignedMipSize << std::endl;
        std::cout << "\t" << "residencyNonResidentStrict = " << properties.sparseProperties.residencyNonResidentStrict << std::endl;
        std::cout << std::endl;
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
            Fail(stream.str().c_str());
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
