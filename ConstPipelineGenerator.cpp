#include "ConstPipelineGenerator.h"

#include <array>

#include "Application.h"
#include "ShaderLoader.h"
#include "VulkanManager.h"

namespace VulkanDemo
{
    ConstPipelineGenerator::ConstPipelineGenerator(VkRenderPass renderPass, uint32_t subpass)
    {
        CreatePipeline(renderPass, subpass);
    }

    ConstPipelineGenerator::~ConstPipelineGenerator()
    {
        DestroyPipeline();
    }

    void ConstPipelineGenerator::CreatePipeline(VkRenderPass renderPass, uint32_t subpass)
    {
        VkDevice device = Application::GetInstance().GetVulkanManager()->GetDevice();

        // TODO: This should be metadata-driven.
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        {
            std::array<VkPipelineShaderStageCreateInfo, 2> stageCreateInfos;
            {
                stageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageCreateInfos[0].pNext = NULL;
                stageCreateInfos[0].flags = 0;
                stageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
                stageCreateInfos[0].module = Application::GetInstance().GetShaderLoader()->GetConstVert();
                stageCreateInfos[0].pName = "main";
                stageCreateInfos[0].pSpecializationInfo = NULL;

                stageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageCreateInfos[1].pNext = NULL;
                stageCreateInfos[1].flags = 0;
                stageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                stageCreateInfos[1].module = Application::GetInstance().GetShaderLoader()->GetConstFrag();
                stageCreateInfos[1].pName = "main";
                stageCreateInfos[1].pSpecializationInfo = NULL;
            }

            VkPipelineVertexInputStateCreateInfo vertexState{};
            {
                // Vertex Positions
                VkVertexInputBindingDescription inputBindingDescription{};
                {
                    inputBindingDescription.binding = 0;
                    inputBindingDescription.stride = 4 * sizeof(float);
                    inputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                }
                VkVertexInputAttributeDescription inputAttributeDescription{};
                {
                    inputAttributeDescription.location = 0;
                    inputAttributeDescription.binding = 0;
                    inputAttributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                    inputAttributeDescription.offset = 0;
                }

                vertexState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                vertexState.pNext = NULL;
                vertexState.flags = 0;
                vertexState.vertexBindingDescriptionCount = 1;
                vertexState.pVertexBindingDescriptions = &inputBindingDescription;
                vertexState.vertexAttributeDescriptionCount = 1;
                vertexState.pVertexAttributeDescriptions = &inputAttributeDescription;
            }

            VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
            {
                inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssemblyState.pNext = NULL;
                inputAssemblyState.flags = 0;
                inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                inputAssemblyState.primitiveRestartEnable = VK_FALSE;
            }

            VkPipelineViewportStateCreateInfo viewportState{};
            {
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.pNext = NULL;
                viewportState.flags = 0;
                viewportState.viewportCount = 1;
                viewportState.pViewports = nullptr; // Set dynamically.
                viewportState.scissorCount = 1;
                viewportState.pScissors = nullptr; // Set dynamically.
            }

            VkPipelineRasterizationStateCreateInfo rasterizationState{};
            {
                rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizationState.pNext = NULL;
                rasterizationState.flags = 0;
                rasterizationState.depthClampEnable = VK_FALSE;
                rasterizationState.rasterizerDiscardEnable = VK_FALSE;
                rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizationState.cullMode = VK_CULL_MODE_NONE; // Not required for blit.
                rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizationState.depthBiasEnable = VK_FALSE;
                rasterizationState.depthBiasConstantFactor = 0.0f;
                rasterizationState.depthBiasClamp = 0.0f;
                rasterizationState.depthBiasSlopeFactor = 0.0f;
                rasterizationState.lineWidth = 1.0;
            }

            VkPipelineMultisampleStateCreateInfo multisampleState{};
            {
                multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampleState.pNext = NULL;
                multisampleState.flags = 0;
                multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
                multisampleState.sampleShadingEnable = VK_FALSE;
                multisampleState.minSampleShading = 0;
                multisampleState.pSampleMask = NULL;
                multisampleState.alphaToCoverageEnable = VK_FALSE;
                multisampleState.alphaToOneEnable = VK_FALSE;
            }

            VkPipelineDepthStencilStateCreateInfo depthStencilState{};
            {
                VkStencilOpState opState{};
                {
                    opState.failOp = VK_STENCIL_OP_KEEP;
                    opState.passOp = VK_STENCIL_OP_KEEP;
                    opState.depthFailOp = VK_STENCIL_OP_KEEP;
                    opState.compareOp = VK_COMPARE_OP_ALWAYS;
                    opState.compareMask = 0;
                    opState.writeMask = 0;
                    opState.reference = 0;
                }

                depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencilState.pNext = NULL;
                depthStencilState.flags = 0;
                depthStencilState.depthTestEnable = VK_FALSE;
                depthStencilState.depthWriteEnable = VK_FALSE;
                depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
                depthStencilState.depthBoundsTestEnable = VK_FALSE;
                depthStencilState.stencilTestEnable = VK_FALSE;
                depthStencilState.front = opState;
                depthStencilState.back = opState;
                depthStencilState.minDepthBounds = 0;
                depthStencilState.maxDepthBounds = 0;
            }

            VkPipelineColorBlendStateCreateInfo colorBlendState{};
            {
                VkPipelineColorBlendAttachmentState attachmentState{};
                {
                    // Ovewrite with source color and alpha.
                    attachmentState.blendEnable = VK_FALSE;
                    attachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                    attachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    attachmentState.colorBlendOp = VK_BLEND_OP_ADD;
                    attachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    attachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                    attachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
                    attachmentState.colorWriteMask =
                        VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT;
                }

                colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlendState.pNext = NULL;
                colorBlendState.flags = 0;
                colorBlendState.logicOpEnable = VK_FALSE;
                colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
                colorBlendState.attachmentCount = 1;
                colorBlendState.pAttachments = &attachmentState;
                colorBlendState.blendConstants[0] = 0;
                colorBlendState.blendConstants[1] = 0;
                colorBlendState.blendConstants[2] = 0;
                colorBlendState.blendConstants[3] = 0;
            }

            VkPipelineDynamicStateCreateInfo dynamicState{};
            {
                std::array<VkDynamicState, 2> dynamicStates =
                {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR
                };

                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.pNext = NULL;
                dynamicState.flags = 0;
                dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
                dynamicState.pDynamicStates = dynamicStates.data();
            }

            // Create the pipeline layout.
            {
                VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
                {
                    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                    pipelineLayoutCreateInfo.pNext = NULL;
                    pipelineLayoutCreateInfo.flags = 0;
                    pipelineLayoutCreateInfo.setLayoutCount = 0;
                    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
                    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
                    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
                }

                CheckResult(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, &m_PipelineLayout));
            }

            pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineCreateInfo.pNext = NULL;
            pipelineCreateInfo.flags = 0;
            pipelineCreateInfo.stageCount = (uint32_t)stageCreateInfos.size();
            pipelineCreateInfo.pStages = stageCreateInfos.data();
            pipelineCreateInfo.pVertexInputState = &vertexState;
            pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
            pipelineCreateInfo.pTessellationState = nullptr;
            pipelineCreateInfo.pViewportState = &viewportState;
            pipelineCreateInfo.pRasterizationState = &rasterizationState;
            pipelineCreateInfo.pMultisampleState = &multisampleState;
            pipelineCreateInfo.pDepthStencilState = &depthStencilState;
            pipelineCreateInfo.pColorBlendState = &colorBlendState;
            pipelineCreateInfo.pDynamicState = &dynamicState;
            pipelineCreateInfo.layout = m_PipelineLayout;
            pipelineCreateInfo.renderPass = renderPass;
            pipelineCreateInfo.subpass = subpass;
            pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineCreateInfo.basePipelineIndex = 0;
        }

        CheckResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &m_Pipeline));
    }

    void ConstPipelineGenerator::DestroyPipeline()
    {
        VkDevice device = Application::GetInstance().GetVulkanManager()->GetDevice();

        vkDestroyPipeline(device, m_Pipeline, NULL);
        m_Pipeline = VK_NULL_HANDLE;

        vkDestroyPipelineLayout(device, m_PipelineLayout, NULL);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
}
