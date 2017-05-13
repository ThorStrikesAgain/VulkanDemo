#pragma once

#include "Shared.h"

namespace VulkanDemo
{
    class Configuration
    {
    public:
        // UNORM is REQUIRED
        static const VkFormat ForwardRendererColorFormat = VK_FORMAT_R8G8B8A8_UNORM;

        // UNORM/UINT is REQUIRED
        static const VkFormat ForwardRendererDepthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    };
}