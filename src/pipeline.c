/*

This file is part of antler.
Copyright (C) 2024 Taylor Wampler 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
  
*/

#include "antler.h"
#include <stdio.h>

static const VkDynamicState dynamicStates[] =
{
  VK_DYNAMIC_STATE_VIEWPORT,
  VK_DYNAMIC_STATE_SCISSOR
};

VkShaderModule atlrInitShaderModule(const char* restrict path, const AtlrDevice* restrict device)
{
  FILE* file = fopen(path, "rb");
  if (!file)
  {
    ATLR_ERROR_MSG("Failed to open file at path \"%s\".", path);
    return VK_NULL_HANDLE;
  }
  fseek(file, 0, SEEK_END);
  const long int codeSize = ftell(file);
  rewind(file);
  char* bytes = malloc((codeSize + 1) * sizeof(char));
  bytes[codeSize] = '\0';
  fread(bytes, codeSize, 1, file);
  fclose(file);

  VkShaderModule module;
  AtlrU32* code = (AtlrU32*)bytes;
  const VkShaderModuleCreateInfo moduleInfo =
  {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .codeSize = codeSize,
    .pCode = code
  };
  if (vkCreateShaderModule(device->logical, &moduleInfo, device->instance->allocator, &module) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateShaderModule did not return VK_SUCCESS.");
    free(bytes);
    return VK_NULL_HANDLE;
  }

  free(bytes);
  return module;
}

void atlrDeinitShaderModule(const VkShaderModule module, const AtlrDevice* restrict device)
{
  vkDestroyShaderModule(device->logical, module, device->instance->allocator);
}

VkPipelineShaderStageCreateInfo atlrInitPipelineVertexShaderStageInfo(const VkShaderModule module)
{
  return (VkPipelineShaderStageCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .pName = "main",
    .module = module,
    .pSpecializationInfo = NULL
  };
}

VkPipelineShaderStageCreateInfo atlrInitPipelineFragmentShaderStageInfo(const VkShaderModule module)
{
  return (VkPipelineShaderStageCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .pName = "main",
    .module = module,
    .pSpecializationInfo = NULL
  };
}

VkPipelineShaderStageCreateInfo atlrInitPipelineComputeShaderStageInfo(const VkShaderModule module)
{
  return (VkPipelineShaderStageCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
    .pName = "main",
    .module = module,
    .pSpecializationInfo = NULL
  };
}

VkPipelineVertexInputStateCreateInfo atlrInitVertexInputStateInfo(const AtlrU32 bindingCount, const VkVertexInputBindingDescription* restrict bindings,
								  const AtlrU32 attributeCount, const VkVertexInputAttributeDescription* restrict attributes)
{
  return (VkPipelineVertexInputStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .vertexBindingDescriptionCount = bindingCount,
    .pVertexBindingDescriptions = bindings,
    .vertexAttributeDescriptionCount = attributeCount,
    .pVertexAttributeDescriptions = attributes
  };
}

VkPipelineInputAssemblyStateCreateInfo atlrInitPipelineInputAssemblyStateInfo()
{
  return (VkPipelineInputAssemblyStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };
}

VkPipelineViewportStateCreateInfo atlrInitPipelineViewportStateInfo()
{
  return (VkPipelineViewportStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .viewportCount = 1,
    .pViewports = NULL,
    .scissorCount = 1,
    .pScissors = NULL
  };
}

VkPipelineRasterizationStateCreateInfo atlrInitPipelineRasterizationStateInfo()
{
  return (VkPipelineRasterizationStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
    .lineWidth = 1.0f
  };
}

VkPipelineMultisampleStateCreateInfo atlrInitPipelineMultisampleStateInfo(const VkSampleCountFlagBits samples)
{
  return (VkPipelineMultisampleStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .rasterizationSamples = samples,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
    .pSampleMask = NULL,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };
}

VkPipelineDepthStencilStateCreateInfo atlrInitPipelineDepthStencilStateInfo()
{
  return (VkPipelineDepthStencilStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL, // reverse-z convention
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .front = {},
    .back = {},
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 0.0f
  };
}

VkPipelineColorBlendAttachmentState atlrInitPipelineColorBlendAttachmentState()
{
  return (VkPipelineColorBlendAttachmentState)
  {
    .blendEnable = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  }; 
}

VkPipelineColorBlendStateCreateInfo atlrInitPipelineColorBlendStateInfo(const VkPipelineColorBlendAttachmentState* restrict attachment)
{
  return (VkPipelineColorBlendStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = attachment,
    .blendConstants = {},
  };
}

VkPipelineDynamicStateCreateInfo atlrInitPipelineDynamicStateInfo()
{
  return (VkPipelineDynamicStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
    .pDynamicStates = dynamicStates
  };
}

VkPipelineLayoutCreateInfo atlrInitPipelineLayoutInfo(const AtlrU32 setLayoutCount, const VkDescriptorSetLayout* restrict setLayouts,
						      const AtlrU32 pushConstantRangeCount, const VkPushConstantRange* restrict pushConstantRanges)
{
  return (VkPipelineLayoutCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .setLayoutCount = setLayoutCount,
    .pSetLayouts = setLayouts,
    .pushConstantRangeCount = pushConstantRangeCount,
    .pPushConstantRanges = pushConstantRanges
  };
}

AtlrU8 atlrInitGraphicsPipeline(AtlrPipeline* restrict pipeline,
				const AtlrU32 stageCount, const VkPipelineShaderStageCreateInfo* restrict stageInfos,
				const VkPipelineVertexInputStateCreateInfo* restrict vertexInputInfo,
				const VkPipelineInputAssemblyStateCreateInfo* restrict inputAssemblyInfo,
				const VkPipelineTessellationStateCreateInfo* restrict tessellationInfo,
				const VkPipelineViewportStateCreateInfo* restrict viewportInfo,
				const VkPipelineRasterizationStateCreateInfo* restrict rasterizationInfo,
				const VkPipelineMultisampleStateCreateInfo* restrict multisampleInfo,
				const VkPipelineDepthStencilStateCreateInfo* restrict depthStencilInfo,
				const VkPipelineColorBlendStateCreateInfo* restrict colorBlendInfo,
				const VkPipelineDynamicStateCreateInfo* restrict dynamicInfo,
				const VkPipelineLayoutCreateInfo* restrict pipelineLayoutInfo,
				const AtlrDevice* restrict device, const AtlrRenderPass* restrict renderPass)
{
  pipeline->device = device;
  
  if (vkCreatePipelineLayout(device->logical, pipelineLayoutInfo, device->instance->allocator, &pipeline->layout) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreatePipelineLayout did not return VK_SUCCESS.");
    return 0;
  }

  const VkGraphicsPipelineCreateInfo pipelineInfo =
  {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stageCount = stageCount,
    .pStages = stageInfos,
    .pVertexInputState = vertexInputInfo,
    .pInputAssemblyState = inputAssemblyInfo,
    .pTessellationState = tessellationInfo,
    .pViewportState = viewportInfo,
    .pRasterizationState = rasterizationInfo,
    .pMultisampleState = multisampleInfo,
    .pDepthStencilState = depthStencilInfo,
    .pColorBlendState = colorBlendInfo,
    .pDynamicState = dynamicInfo,
    .layout = pipeline->layout,
    .renderPass = renderPass->renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };
  if (vkCreateGraphicsPipelines(device->logical, VK_NULL_HANDLE, 1, &pipelineInfo, device->instance->allocator, &pipeline->pipeline) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateGraphicsPipelines did not return VK_SUCCESS.");
    return 0;
  }

  pipeline->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  return 1;
}

AtlrU8 atlrInitComputePipeline(AtlrPipeline* restrict pipeline,
			       const VkPipelineShaderStageCreateInfo* restrict stageInfo,
			       const VkPipelineLayoutCreateInfo* restrict pipelineLayoutInfo,
			       const AtlrDevice* restrict device)
{
  pipeline->device = device;
  
  if (vkCreatePipelineLayout(device->logical, pipelineLayoutInfo, device->instance->allocator, &pipeline->layout) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreatePipelineLayout did not return VK_SUCCESS.");
    return 0;
  }

  const VkComputePipelineCreateInfo pipelineInfo =
  {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = *stageInfo,
    .layout = pipeline->layout,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };
  if (vkCreateComputePipelines(device->logical, VK_NULL_HANDLE, 1, &pipelineInfo, device->instance->allocator, &pipeline->pipeline) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateComputePipelines did not return VK_SUCCESS.");
    return 0;
  }

  pipeline->bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

  return 1;
}

void atlrDeinitPipeline(const AtlrPipeline* restrict pipeline)
{
  const AtlrDevice* device = pipeline->device;
  vkDestroyPipelineLayout(device->logical, pipeline->layout, device->instance->allocator);
  vkDestroyPipeline(device->logical, pipeline->pipeline, device->instance->allocator);
}
