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
#include "../../src/transforms.h"

typedef struct ColorVertex
{
  AtlrVec2 pos;
  AtlrVec3 color;
  
} ColorVertex;

typedef struct Pipeline
{
  VkPipelineLayout layout;
  VkPipeline pipeline;
  
} Pipeline;

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSwapchain swapchain;
static AtlrFrameCommandContext commandContext;
static AtlrMesh quadMesh;
static Pipeline pipeline;

static AtlrU8 initPipeline()
{
  VkShaderModule modules[2] =
  {
    atlrInitShaderModule("quad-vert.spv", &device),
    atlrInitShaderModule("quad-frag.spv", &device)
  };
  VkPipelineShaderStageCreateInfo stageInfos[2] =
  {
    atlrInitPipelineVertexShaderStageInfo(modules[0]),
    atlrInitPipelineFragmentShaderStageInfo(modules[1])
  };

  const VkVertexInputBindingDescription vertexInputBindingDescription =
  {
    .binding = 0,
    .stride = sizeof(ColorVertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };
  const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
  {
    (VkVertexInputAttributeDescription)
    {
      .location = 0,
      .binding = 0,
      .format = VK_FORMAT_R32G32_SFLOAT,
      .offset = offsetof(ColorVertex, pos)
    },
    (VkVertexInputAttributeDescription)
    {
      .location = 1,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = offsetof(ColorVertex, color)
    }
  };
  const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexInputBindingDescription,
    .vertexAttributeDescriptionCount = 2,
    .pVertexAttributeDescriptions = vertexInputAttributeDescriptions
  };

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

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(0, NULL, 0, NULL);
  if (vkCreatePipelineLayout(device.logical, &pipelineLayoutInfo, instance.allocator, &pipeline.layout) != VK_SUCCESS)
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
    .layout = pipeline.layout,
    .renderPass = swapchain.renderPass.renderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };
  if (vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineInfo, instance.allocator, &pipeline.pipeline) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateGraphicsPipelines did not return VK_SUCCESS.");
    return 0;
  }

  atlrDeinitShaderModule(modules[0], &device);
  atlrDeinitShaderModule(modules[1], &device);
  
  return 1;
}

static void deinitPipeline()
{
  vkDestroyPipelineLayout(device.logical, pipeline.layout, instance.allocator);
  vkDestroyPipeline(device.logical, pipeline.pipeline, instance.allocator);
}
  
static AtlrU8 initHelloQuad()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Hello Quad' demo ...");

  if (!atlrInitInstanceHostGLFW(&instance, 800, 400, "Hello Quad Demo"))
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

  AtlrSingleRecordCommandContext singleRecordCommandContext;
  if (!atlrInitSingleRecordCommandContext(&singleRecordCommandContext, device.queueFamilyIndices.graphicsComputeIndex, &device))
  {
    ATLR_ERROR_MSG("atlrInitSingleRecordCommandContext returned 0.");
    return 0;
  }
  
  const ColorVertex quadVertices[4] =
  {
    (ColorVertex){.pos = {{-0.5f, -0.5f}}, .color = {{1.0f, 0.0f, 0.0f}}},
    (ColorVertex){.pos = {{ 0.5f, -0.5f}}, .color = {{0.0f, 1.0f, 0.0f}}},
    (ColorVertex){.pos = {{ 0.5f,  0.5f}}, .color = {{0.0f, 0.0f, 1.0f}}},
    (ColorVertex){.pos = {{-0.5f,  0.5f}}, .color = {{1.0f, 0.0f, 1.0f}}}
  };
  const AtlrU16 indices[6] =
  {
    0, 1, 2, 2, 3, 0
  };
  AtlrU64 quadVerticesSize = 4 * sizeof(ColorVertex);

  if (!atlrInitMesh(&quadMesh, quadVerticesSize, quadVertices, 6, indices, &device, &singleRecordCommandContext))
  {
    ATLR_ERROR_MSG("atlrInitMesh returned 0.");
    return 0;
  }
  
  atlrDeinitSingleRecordCommandContext(&singleRecordCommandContext);

  if (!initPipeline())
  {
    ATLR_ERROR_MSG("initPipeline returned 0.");
    return 0;
  }

  return 1;
}

static void deinitHelloQuad()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Hello Quad' demo ...");

  vkDeviceWaitIdle(device.logical);
  
  deinitPipeline();
  atlrDeinitMesh(&quadMesh);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
}


int main()
{
  if (!initHelloQuad())
  {
    ATLR_FATAL_MSG("initHelloQuad returned 0.");
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
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    atlrBindMesh(&quadMesh, commandBuffer);
    atlrDrawMesh(&quadMesh, commandBuffer);

    if (!atlrEndFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrEndFrameCommands returned 0.");
      return -1;
    }
  }

  deinitHelloQuad();

  return 0;
}
