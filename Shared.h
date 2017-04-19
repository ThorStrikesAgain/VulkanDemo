#pragma once

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#include <functional>

namespace VulkanDemo
{
    ///
    /// If BUILD_ENABLE_RUNTIME_DEBUG is enabled, when result is negative, the error will be logged, a message box will
    /// be displayed a message box is displayed and the application will exit.
    /// 
    void CheckResult(VkResult result);

    ///
    /// Displays a table in the console with optional headers.
    ///
    /// @param[in] colCount     Number of columns of data to be displayed.
    /// @param[in] rowCount     Number of rows of data to be displayed.
    /// @param[in] colHeaders   Headers for the columns. Assign nullptr to display no headers.
    /// @param[in] getItem      Function used to retrieve the data item at the given row and column.
    ///
    void PrintTable(int colCount, int rowCount, const char * const * colHeaders, std::function<const char *(int row, int col)> getItem);

    ///
    /// Logs the message of a fatal error and exits the application with the given code.
    ///
    void Fail(const char* message, int code);
} // VulkanDemo
