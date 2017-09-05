#pragma once

#include "Shared.h"

namespace VulkanDemo
{
    ///
    /// The purpose of this class is to help in the generation of a graphics pipeline intended to blit a texture.
    /// It keeps ownership of the created objects and handles their destruction properly.
    ///
    /// Usage Notes:
    /// - Dynamic states that must be set externally: viewport and scissors.
    /// - An unmutable sampler is defined and a corresponding texture needs to be bound.
    ///
    class BlitPipelineGenerator
    {
    public:
        BlitPipelineGenerator(VkRenderPass renderPass, uint32_t subpass);
        ~BlitPipelineGenerator();

        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
        VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

    private:
        BlitPipelineGenerator & operator=(const BlitPipelineGenerator&) = delete;
        BlitPipelineGenerator(const BlitPipelineGenerator&) = delete;

        void CreatePipeline(VkRenderPass renderPass, uint32_t subpass);
        void DestroyPipeline();

        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };
}
