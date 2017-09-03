#pragma once

#include "Shared.h"

namespace VulkanDemo
{
    ///
    /// The purpose of this class is to help in the generation of a graphics pipeline intended to blit a constant
    /// color. It keeps ownership of the created objects and handles their destruction properly.
    ///
    /// Usage Notes:
    /// Dynamic states that must be set externally: viewport and scissors.
    ///
    class ConstPipelineGenerator
    {
    public:
        ConstPipelineGenerator(VkRenderPass renderPass, uint32_t subpass);
        ~ConstPipelineGenerator();

        VkPipeline GetPipeline() const { return m_Pipeline; }

    private:
        ConstPipelineGenerator & operator=(const ConstPipelineGenerator&) = delete;
        ConstPipelineGenerator(const ConstPipelineGenerator&) = delete;

        void CreatePipeline(VkRenderPass renderPass, uint32_t subpass);
        void DestroyPipeline();

        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    };
}
