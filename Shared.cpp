#include "Shared.h"

#include <cassert>

#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <vector>

#include "BUILD_OPTIONS.h"

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
        
        MessageBox(NULL, "A Vulkan runtime error has occurred.", "Fatal Error", MB_OK | MB_ICONEXCLAMATION);
        exit(-1);
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
            widths[col] = max(widths[col], (int)strlen(item));
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
