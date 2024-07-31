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
#include <stdio.h>
#include <time.h>

typedef struct Vertex
{
  AtlrF32 pos[2];
  
} Vertex;

typedef struct Transfom
{
  AtlrF32 translate[2];
  AtlrF32 scale[2];
  
} Transform;

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
    .stride = sizeof(Vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };
  const VkVertexInputAttributeDescription vertexInputAttributeDescription =
  {
    .location = 0,
    .binding = 0,
    .format = VK_FORMAT_R32G32_SFLOAT,
    .offset = offsetof(Vertex, pos)
  };
  const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexInputBindingDescription,
    .vertexAttributeDescriptionCount = 1,
    .pVertexAttributeDescriptions = &vertexInputAttributeDescription
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

  const VkPushConstantRange pushConstantRange =
  {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .offset = 0,
    .size = sizeof(Transform)
  };
  VkPipelineLayoutCreateInfo pipelineLayoutInfo =
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .setLayoutCount = 0,
    .pSetLayouts = NULL,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &pushConstantRange
  };
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
  
static AtlrU8 initConwayLife()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Conway's Game of Life' demo ...");

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
  
  const Vertex quadVertices[4] =
  {
    (Vertex){.pos = {0.1f, 0.1f}},
    (Vertex){.pos = {0.9f, 0.1f}},
    (Vertex){.pos = {0.9f, 0.9f}},
    (Vertex){.pos = {0.1f, 0.9f}}
  };
  const AtlrU16 indices[6] =
  {
    0, 1, 2, 2, 3, 0
  };
  AtlrU64 quadVerticesSize = 4 * sizeof(Vertex);

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

static void deinitConwayLife()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Conway's Game of Life' demo ...");

  vkDeviceWaitIdle(device.logical);
  
  deinitPipeline();
  atlrDeinitMesh(&quadMesh, &device);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
}

// update cells based on the rules of Conway's Game of Life
static void updateCells(AtlrU8* restrict cells, const AtlrU8* restrict oldCells, const AtlrU32 rows, const AtlrU32 columns)
{
  for (AtlrU32 i = 0; i < columns; i++)
    for (AtlrU32 j = 0; j < rows; j++)
      {
	AtlrU8 count = 0;
	if (i > 0)
	{
	  if (j > 0)
	    count += oldCells[(i - 1) * rows + (j - 1)];
	  if (j < rows - 1)
	    count += oldCells[(i - 1) * rows + (j + 1)];
	  count += oldCells[(i - 1) * rows + j];
	}
	if (i < columns - 1)
	{
	  if (j > 0)
	    count += oldCells[(i + 1) * rows + (j - 1)];
	  if (j < rows - 1)
	    count += oldCells[(i + 1) * rows + (j + 1)];
	  count += oldCells[(i + 1) * rows + j];
	}
	if (j > 0)
	  count += oldCells[i * rows + (j - 1)];
	if (j < rows - 1)
	  count += oldCells[i * rows + (j + 1)];

	if (oldCells[i * rows + j])
	{
	  if (count < 2 || count > 3)
	    cells[i * rows + j] = 0;
	}
	else if (count == 3)
	  cells[i * rows + j] = 1;
      }
}

int main()
{
  unsigned int seed;
  AtlrU32 rows, columns;
  printf("Enter a seed value: ");
  scanf("%u", &seed);
  printf("Row count: ");
  scanf("%u", &rows);
  printf("Column count: ");
  scanf("%u", &columns);

  atlrLog(ATLR_LOG_INFO, "Playing the game of life  ...");
  
  srand(seed);

  Transform transform;
  transform.scale[0] = 2.0f / columns;
  transform.scale[1] = 2.0f / rows;
  
  AtlrU8* cells = malloc(rows * columns * sizeof(AtlrU8));
  AtlrU8* oldCells = malloc(rows * columns * sizeof(AtlrU8));
  for (AtlrU32 i = 0; i < columns; i++)
    for (AtlrU32 j = 0; j < rows; j++)
      cells[i * rows + j] = (AtlrU8)(rand() & 1);
  memcpy(oldCells, cells, rows * columns * sizeof(AtlrU8));

  if (!initConwayLife())
  {
    ATLR_FATAL_MSG("initConwayLife returned 0.");
    return -1;
  }

  clock_t startTime = clock();
  const double interval = 0.8;
  GLFWwindow* window = instance.data;
  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    
    clock_t currentTime = clock();
    double elapsedTime = (double)(currentTime - startTime) / CLOCKS_PER_SEC;
    if (elapsedTime >= interval)
    {
      updateCells(cells, oldCells, rows, columns);
      memcpy(oldCells, cells, rows * columns * sizeof(AtlrU8));
      startTime = clock();
    }
    
    if (!atlrBeginFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrBeginFrameCommands returned 0.");
      return -1;
    }

    const VkCommandBuffer commandBuffer = atlrGetFrameCommandContextCommandBufferHostGLFW(&commandContext);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    atlrBindMesh(&quadMesh, commandBuffer);

    for (AtlrU32 i = 0; i < columns; i++)
    {
      transform.translate[0] = -1.0f + i * transform.scale[0];
      for (AtlrU32 j = 0; j < rows; j++)
      {
	if (!cells[i * rows + j]) continue;

	transform.translate[1] = -1.0f + j * transform.scale[1];
	vkCmdPushConstants(commandBuffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Transform), &transform);
	atlrDrawMesh(&quadMesh, commandBuffer);
      }
    }

    if (!atlrEndFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrEndFrameCommands returned 0.");
      return -1;
    }
  }

  free(cells);
  free(oldCells);

  deinitConwayLife();

  return 0;
}
