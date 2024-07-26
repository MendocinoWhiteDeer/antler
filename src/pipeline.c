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
  FILE* file = fopen(path, "rb");;
  if (!file)
  {
    ATLR_LOG_ERROR("Failed to open file at path \"%s\".", path);
    return VK_NULL_HANDLE;
  }
  fseek(file, 0, SEEK_END);
  const long int codeSize = ftell(file);
  rewind(file);
  char* bytes = malloc((codeSize + 1) * sizeof(char));
  fread(bytes, codeSize, 1, file);
  fclose(file);

  VkShaderModule module;
  AtlrU32* code = (AtlrU32*)bytes;
  const VkShaderModuleCreateInfo moduleInfo =
  {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .codeSize = codeSize,
    .pCode = code
  };
  if (vkCreateShaderModule(device->logical, &moduleInfo, device->instance->allocator, &module) != VK_SUCCESS)
  {
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

VkPipelineMultisampleStateCreateInfo atlrInitPipelineMultisampleStateInfo()
{
  return (VkPipelineMultisampleStateCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
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
    .depthCompareOp = VK_COMPARE_OP_GREATER, // reverse-z convention
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

VkPipelineLayoutCreateInfo atlrInitPipelineLayoutInfo(AtlrU32 setLayoutCount, const VkDescriptorSetLayout* restrict setLayouts)
{
  return (VkPipelineLayoutCreateInfo)
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .setLayoutCount = setLayoutCount,
    .pSetLayouts = setLayouts,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL
  };
}
