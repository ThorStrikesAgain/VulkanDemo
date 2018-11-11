#pragma once

#include "Shared.h"

namespace VulkanDemo
{
    class Configuration
    {
    public:
        // Linear & HDR rendering
        static const VkFormat SceneColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        // UNORM/UINT is REQUIRED
        static const VkFormat SceneDepthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    };
}