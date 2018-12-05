#include "Shared.h"

#include <cassert>

#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include "BUILD_OPTIONS.h"

#include "Application.h"
#include "VulkanManager.h"

namespace VulkanDemo
{
    VkDeviceMemory FindAndAllocateMemory(VkMemoryRequirements const & requirements, VkMemoryPropertyFlags requiredFlags)
    {
        VulkanManager * vulkanManager = Application::GetInstance().GetVulkanManager();

        // List the types that support the properties we want.
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(vulkanManager->GetPhysicalDevice(), &physicalDeviceMemoryProperties);
        for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
        {
            if (((1 << i) & requirements.memoryTypeBits) == 0)
            {
                // This memory type cannot be used based on the requirements.
                continue;
            }

            VkMemoryPropertyFlags flags = physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags;
            if ((flags & requiredFlags) != requiredFlags)
            {
                // This memory type does not match our requirements.
                continue;
            }

            VkMemoryAllocateInfo allocateInfo{};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.pNext = NULL;
            allocateInfo.allocationSize = requirements.size;
            allocateInfo.memoryTypeIndex = i;

            VkDeviceMemory memory;
            CheckResult(vkAllocateMemory(vulkanManager->GetDevice(), &allocateInfo, NULL, &memory));
            return memory;
        }

        return VK_NULL_HANDLE;
    }

    VkDeviceMemory AllocateAndBindBufferMemory(VkBuffer buffer)
    {
        assert(buffer != VK_NULL_HANDLE);

        VulkanManager * vulkanManager = Application::GetInstance().GetVulkanManager();

        // List the types that can be used for the buffer.
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(vulkanManager->GetDevice(), buffer, &requirements);

        // Allocate memory.
        VkDeviceMemory memory = FindAndAllocateMemory(
            requirements,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (memory == VK_NULL_HANDLE)
        {
            Fail("Failed to find a suitable memory type.");
        }

        // Bind memory.
        CheckResult(vkBindBufferMemory(vulkanManager->GetDevice(), buffer, memory, 0));
        return memory;
    }

    VkDeviceMemory AllocateAndBindImageMemory(VkImage image)
    {
        assert(image != VK_NULL_HANDLE);

        VulkanManager * vulkanManager = Application::GetInstance().GetVulkanManager();

        // List the types that can be used for the image.
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(vulkanManager->GetDevice(), image, &requirements);

        // Allocate memory.
        VkDeviceMemory memory = FindAndAllocateMemory(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memory == VK_NULL_HANDLE)
        {
            Fail("Failed to find a suitable memory type.");
        }

        // Bind memory.
        CheckResult(vkBindImageMemory(vulkanManager->GetDevice(), image, memory, 0));
        return memory;
    }

#if BUILD_ENABLE_RUNTIME_DEBUG
    void CheckResult(VkResult result)
    {
        if (result < 0)
        {
            switch (result)
            {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                std::cout << "VK_ERROR_OUT_OF_HOST_MEMORY" << std::endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                std::cout << "VK_ERROR_OUT_OF_DEVICE_MEMORY" << std::endl;
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                std::cout << "VK_ERROR_INITIALIZATION_FAILED" << std::endl;
                break;
            case VK_ERROR_DEVICE_LOST:
                std::cout << "VK_ERROR_DEVICE_LOST" << std::endl;
                break;
            case VK_ERROR_MEMORY_MAP_FAILED:
                std::cout << "VK_ERROR_MEMORY_MAP_FAILED" << std::endl;
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                std::cout << "VK_ERROR_LAYER_NOT_PRESENT" << std::endl;
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                std::cout << "VK_ERROR_EXTENSION_NOT_PRESENT" << std::endl;
                break;
            case VK_ERROR_FEATURE_NOT_PRESENT:
                std::cout << "VK_ERROR_FEATURE_NOT_PRESENT" << std::endl;
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                std::cout << "VK_ERROR_INCOMPATIBLE_DRIVER" << std::endl;
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                std::cout << "VK_ERROR_TOO_MANY_OBJECTS" << std::endl;
                break;
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                std::cout << "VK_ERROR_FORMAT_NOT_SUPPORTED" << std::endl;
                break;
            case VK_ERROR_FRAGMENTED_POOL:
                std::cout << "VK_ERROR_FRAGMENTED_POOL" << std::endl;
                break;
            case VK_ERROR_SURFACE_LOST_KHR:
                std::cout << "VK_ERROR_SURFACE_LOST_KHR" << std::endl;
                break;
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                std::cout << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" << std::endl;
                break;
            case VK_SUBOPTIMAL_KHR:
                std::cout << "VK_SUBOPTIMAL_KHR" << std::endl;
                break;
            case VK_ERROR_OUT_OF_DATE_KHR:
                std::cout << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
                break;
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                std::cout << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" << std::endl;
                break;
            case VK_ERROR_VALIDATION_FAILED_EXT:
                std::cout << "VK_ERROR_VALIDATION_FAILED_EXT" << std::endl;
                break;
            case VK_ERROR_INVALID_SHADER_NV:
                std::cout << "VK_ERROR_INVALID_SHADER_NV" << std::endl;
                break;
            case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
                std::cout << "VK_ERROR_OUT_OF_POOL_MEMORY_KHR" << std::endl;
                break;
            default:
                break;
            }

            Fail("A Vulkan runtime error has occurred.");
        }
    }

#else

    void CheckResult(VkResult result)
    {
    }

#endif // BUILD_ENABLE_RUNTIME_DEBUG

    void PrintTable(int colCount, int rowCount, const char * const * colHeaders, std::function<const char *(int, int)> getItem)
    {
        if (colHeaders != nullptr)
        {
            // Call recursively, adding the headers.
            PrintTable(colCount, rowCount + 1, nullptr, [getItem, colHeaders](int row, int col) {
                if (row == 0)
                {
                    return colHeaders[col];
                }
                return getItem(row - 1, col);
            });
            return;
        }

        // Calculate widths
        std::vector<int> widths(colCount, 0);
        for (int col = 0; col < colCount; ++col)
        {
            for (int row = 0; row < rowCount; ++row)
            {
                const char * item = getItem(row, col);
                widths[col] = std::max(widths[col], (int)strlen(item));
            }
        }

        // Print
        for (int row = 0; row < rowCount; ++row)
        {
            std::cout << std::left;
            for (int col = 0; col < colCount; ++col)
            {
                std::cout << std::setw(widths[col] + 1) << getItem(row, col);
            }
            std::cout << std::endl;
        }
    }

    void Fail(const char * message, int code)
    {
        std::cout << "Fatal Error: " << message << std::endl;
        MessageBox(NULL, message, "Fatal Error", MB_OK | MB_ICONEXCLAMATION);
        exit(code);
    }
} // VulkanDemo
