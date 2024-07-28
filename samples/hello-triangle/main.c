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

#include "../../src/antler.h"

typedef struct Pipeline
{
  VkPipelineLayout layout;
  VkPipeline pipeline;
  
} Pipeline;

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSwapchain swapchain;
static AtlrFrameCommandContext commandContext;
static Pipeline trianglePipeline;

static AtlrU8 initPipeline(Pipeline* restrict pipeline)
{
  VkShaderModule modules[2] =
  {
    atlrInitShaderModule("triangle-vert.spv", &device),
    atlrInitShaderModule("triangle-frag.spv", &device)
  };
  VkPipelineShaderStageCreateInfo stageInfos[2] =
  {
    atlrInitPipelineVertexShaderStageInfo(modules[0]),
    atlrInitPipelineFragmentShaderStageInfo(modules[1])
  };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo =
    atlrInitPipelineInputAssemblyStateInfo();
  const VkPipelineViewportStateCreateInfo viewportInfo =
    atlrInitPipelineViewportStateInfo();
  const VkPipelineRasterizationStateCreateInfo rasterizationInfo =
    atlrInitPipelineRasterizationStateInfo();
  const VkPipelineMultisampleStateCreateInfo multisampleInfo =
    atlrInitPipelineMultisampleStateInfo();
  const VkPipelineDepthStencilStateCreateInfo depthStencilInfo =
    atlrInitPipelineDepthStencilStateInfo();
  const VkPipelineColorBlendAttachmentState colorBlendAttachment =
    atlrInitPipelineColorBlendAttachmentState();
  const VkPipelineColorBlendStateCreateInfo colorBlendInfo =
    atlrInitPipelineColorBlendStateInfo(&colorBlendAttachment);
  const VkPipelineDynamicStateCreateInfo dynamicInfo =
    atlrInitPipelineDynamicStateInfo();

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    atlrInitPipelineLayoutInfo(0, NULL);
  if (vkCreatePipelineLayout(device.logical, &pipelineLayoutInfo, instance.allocator, &pipeline->layout) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreatePipelineLayout did not return VK_SUCCESS.");
    return 0;
  }

  const VkGraphicsPipelineCreateInfo pipelineInfo =
  {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stageCount = 2,
    .pStages = stageInfos,
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssemblyInfo,
    .pTessellationState = NULL,
    .pViewportState = &viewportInfo,
    .pRasterizationState = &rasterizationInfo,
    .pMultisampleState = &multisampleInfo,
    .pDepthStencilState = &depthStencilInfo,
    .pColorBlendState = &colorBlendInfo,
    .pDynamicState = &dynamicInfo,
    .layout = pipeline->layout,
    .renderPass = swapchain.renderPass.renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };
  if (vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineInfo, instance.allocator, &pipeline->pipeline) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateGraphicsPipelines did not return VK_SUCCESS.");
    return 0;
  }

  atlrDeinitShaderModule(modules[0], &device);
  atlrDeinitShaderModule(modules[1], &device);

  return 1;
}

static void deinitPipeline(Pipeline* restrict pipeline)
{
  vkDestroyPipelineLayout(device.logical, pipeline->layout, instance.allocator);
  vkDestroyPipeline(device.logical, pipeline->pipeline, instance.allocator);
}
  
static AtlrU8 initHelloTriangle()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Hello Triangle' demo ...");

  if (!atlrInitInstanceHostGLFW(&instance, 800, 400, "Hello Triangle Demo"))
  {
    ATLR_ERROR_MSG("atlrInitHostGLFW returned 0.");
    return 0;
  }

  AtlrDeviceCriteria deviceCriteria;
  atlrInitDeviceCriteria(deviceCriteria);
  atlrSetDeviceCriterion(deviceCriteria,
			 ATLR_DEVICE_CRITERION_QUEUE_FAMILY_GRAPHICS_SUPPORT,
			 ATLR_DEVICE_CRITERION_METHOD_REQUIRED,
			 0);
  atlrSetDeviceCriterion(deviceCriteria,
			 ATLR_DEVICE_CRITERION_QUEUE_FAMILY_PRESENT_SUPPORT,
			 ATLR_DEVICE_CRITERION_METHOD_REQUIRED,
			 0);
  atlrSetDeviceCriterion(deviceCriteria,
			 ATLR_DEVICE_CRITERION_SWAPCHAIN_SUPPORT,
			 ATLR_DEVICE_CRITERION_METHOD_REQUIRED,
			 0);
  atlrSetDeviceCriterion(deviceCriteria,
			 ATLR_DEVICE_CRITERION_INTEGRATED_GPU_PHYSICAL_DEVICE,
			 ATLR_DEVICE_CRITERION_METHOD_POINT_SHIFT,
			 10);
  if (!atlrInitDeviceHost(&device, &instance, deviceCriteria))
  {
    ATLR_ERROR_MSG("atlrInitDeviceHost returned 0.");
    return 0;
  }

  if (!atlrInitSwapchainHostGLFW(&swapchain, 1, &device))
  {
    ATLR_ERROR_MSG("atlrInitSwapchainHostGLFW returned 0.");
    return 0;
  }

  if (!atlrInitFrameCommandContextHostGLFW(&commandContext, 2, &swapchain))
  {
    ATLR_ERROR_MSG("atlrInitFrameCommandContext returned 0.");
    return 0;
  }

  if (!initPipeline(&trianglePipeline))
  {
    ATLR_ERROR_MSG("initPipeline returned 0.");
    return 0;
  }

  return 1;
}

static void deinitHelloTriangle()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Hello Triangle' demo ...");

  vkDeviceWaitIdle(device.logical);
  
  deinitPipeline(&trianglePipeline);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
}


int main()
{
  if (!initHelloTriangle())
  {
    ATLR_FATAL_MSG("initHelloTriangle returned 0.");
    return -1;
  }
  
  GLFWwindow* window = instance.data;
  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    
    if (!atlrBeginFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrBeginFrameCommands returned 0.");
      return -1;
    }

    const VkCommandBuffer commandBuffer = atlrGetFrameCommandContextCommandBufferHostGLFW(&commandContext);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline.pipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    if (!atlrEndFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrEndFrameCommands returned 0.");
      return -1;
    }
  }

  deinitHelloTriangle();

  return 0;
}
