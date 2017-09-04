#include "GraphicsHelper.h"

#include <cassert>

#include "Application.h"
#include "VulkanManager.h"

namespace VulkanDemo
{
    GraphicsHelper::GraphicsHelper()
    {
    }

    GraphicsHelper::~GraphicsHelper()
    {
        VkDevice device = Application::GetInstance().GetVulkanManager()->GetDevice();

        if (m_blitVertices != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_blitVerticesMemory, NULL);
            m_blitVerticesMemory = VK_NULL_HANDLE;

            vkDestroyBuffer(device, m_blitVertices, NULL);
            m_blitVertices = VK_NULL_HANDLE;
        }
    }

    VkBuffer GraphicsHelper::GetBlitVertices()
    {
        if (m_blitVertices == VK_NULL_HANDLE)
        {
            VkDevice device = Application::GetInstance().GetVulkanManager()->GetDevice();

            float vertices[4][4] =
            {
                { -1, 1, 0, 1 },
                {  1, 1, 0, 1 },
                { -1, -1, 0, 1 },
                {  1,  -1, 0, 1 }
            };

            // Create the buffer.
            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.pNext = NULL;
            bufferCreateInfo.flags = 0;
            bufferCreateInfo.size = sizeof(vertices);
            bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            bufferCreateInfo.queueFamilyIndexCount = 0;
            bufferCreateInfo.pQueueFamilyIndices = nullptr;
            CheckResult(vkCreateBuffer(device, &bufferCreateInfo, NULL, &m_blitVertices));
            
            // Allocate and bind memory.
            m_blitVerticesMemory = AllocateAndBindBufferMemory(m_blitVertices);

            // Set the data.
            void* mappedData;
            CheckResult(vkMapMemory(device, m_blitVerticesMemory, 0, VK_WHOLE_SIZE, 0, &mappedData));
            memcpy(mappedData, vertices, sizeof(vertices));
            vkUnmapMemory(device, m_blitVerticesMemory);
        }
        return m_blitVertices;
    }
}
