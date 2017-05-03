#pragma once

#include "Shared.h"

namespace VulkanDemo
{
    class SceneRenderer
    {
    public:
        SceneRenderer();
        ~SceneRenderer();

        VkImage Render() { return VK_NULL_HANDLE; }
    };
} // VulkanDemo
