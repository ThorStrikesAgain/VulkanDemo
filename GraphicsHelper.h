#pragma once

#include "Shared.h"

namespace VulkanDemo
{
    class GraphicsHelper
    {
    public:
        GraphicsHelper();
        ~GraphicsHelper();

        ///
        /// Returns a buffer view on a buffer that contains 4 vertices intended to be used as a triangle strip.
        ///
        VkBuffer GetBlitVertices();

    private:
        VkBuffer m_blitVertices = VK_NULL_HANDLE;
        VkDeviceMemory m_blitVerticesMemory = VK_NULL_HANDLE;
    };
}
