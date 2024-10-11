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

/*
References:

Games, M. (1970). The fantastic combinations of John Conway’s new solitaire game “life” by Martin Gardner. Scientific American, 223, 120-123.
*/

#include "../../src/antler.h"
#include "../../src/transforms.h"
#include <stdio.h>

typedef struct Vertex
{
  AtlrVec2 pos;
  
} Vertex;

typedef struct Transfom
{
  AtlrVec2 translate;
  AtlrVec2 scale;
  
} Transform;

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSwapchain swapchain;
static AtlrFrameCommandContext commandContext;
static AtlrMesh quadMesh;
static AtlrPipeline pipeline;

static AtlrU8 initPipeline()
{
  VkShaderModule modules[2] =
  {
    atlrInitShaderModule("quad-vert.spv", &device),
    atlrInitShaderModule("quad-frag.spv", &device)
  };
  VkPipelineShaderStageCreateInfo stageInfos[2] =
  {
    atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, modules[0]),
    atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, modules[1])
  };

  const VkVertexInputBindingDescription vertexInputBindingDescription =
  {
    .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };
  const VkVertexInputAttributeDescription vertexInputAttributeDescription =
  {
    .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, pos)
  };
  const VkPipelineVertexInputStateCreateInfo vertexInputInfo = atlrInitVertexInputStateInfo(1, &vertexInputBindingDescription, 1, &vertexInputAttributeDescription);

  const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = atlrInitPipelineInputAssemblyStateInfo();
  const VkPipelineViewportStateCreateInfo viewportInfo           = atlrInitPipelineViewportStateInfo();
  const VkPipelineRasterizationStateCreateInfo rasterizationInfo = atlrInitPipelineRasterizationStateInfo();
  const VkPipelineMultisampleStateCreateInfo multisampleInfo     = atlrInitPipelineMultisampleStateInfo(device.msaaSamples);
  const VkPipelineDepthStencilStateCreateInfo depthStencilInfo   = atlrInitPipelineDepthStencilStateInfo();
  const VkPipelineColorBlendAttachmentState colorBlendAttachment = atlrInitPipelineColorBlendAttachmentStateAlpha();
  const VkPipelineColorBlendStateCreateInfo colorBlendInfo       = atlrInitPipelineColorBlendStateInfo(&colorBlendAttachment);
  const VkPipelineDynamicStateCreateInfo dynamicInfo             = atlrInitPipelineDynamicStateInfo();

  const VkPushConstantRange pushConstantRange =
  {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(Transform)
  };
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(0, NULL, 1, &pushConstantRange);

  if(!atlrInitGraphicsPipeline(&pipeline,
			       2, stageInfos, &vertexInputInfo, &inputAssemblyInfo, NULL, &viewportInfo, &rasterizationInfo, &multisampleInfo, &depthStencilInfo, &colorBlendInfo, &dynamicInfo, &pipelineLayoutInfo,
			       &device, &swapchain.renderPass))
  {
    ATLR_ERROR_MSG("atlrInitGraphicsPipeline returned 0.");
    return 0;
  }

  atlrDeinitShaderModule(modules[0], &device);
  atlrDeinitShaderModule(modules[1], &device);
  
  return 1;
}

static void deinitPipeline()
{
  atlrDeinitPipeline(&pipeline);
}
  
static AtlrU8 initConwayLife()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Conway's Game of Life' demo ...");

  if (!atlrInitInstanceHostGLFW(&instance, 800, 400, "Game of Life Demo"))
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

  const VkClearValue clearColor = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}};
  if (!atlrInitSwapchainHostGLFW(&swapchain, 1, NULL, NULL, &clearColor, &device))
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
    (Vertex){.pos = {{0.1f, 0.1f}}},
    (Vertex){.pos = {{0.9f, 0.1f}}},
    (Vertex){.pos = {{0.9f, 0.9f}}},
    (Vertex){.pos = {{0.1f, 0.9f}}}
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
#ifdef ATLR_DEBUG
  atlrSetMeshName(&quadMesh, "Quad");
#endif
  
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
  atlrDeinitMesh(&quadMesh);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
}

// update cells based on the rules of Conway's Game of Life
static void updateCells(AtlrU8* restrict cells, const AtlrU8* restrict oldCells, const AtlrI32 rows, const AtlrI32 columns)
{
  for (AtlrI32 i = 0; i < columns; i++)
    for (AtlrI32 j = 0; j < rows; j++)
    {
      AtlrU8 count = 0;
      for (AtlrI32 k = i - 1; k <= i + 1; k++)
      {
	if (k < 0 || k >= columns) continue;
	for (AtlrI32 l = j - 1; l <= j + 1; l++)
	{
	  if (l < 0 || l >= rows || (k == i && l == j)) continue;
	  count += oldCells[k * rows + l];
	}
      }  
      
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
  transform.scale.x = 2.0f / columns;
  transform.scale.y = 2.0f / rows;
  
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

  GLFWwindow* window = instance.data;
  glfwSetTime(0.0);
  const double interval = 0.8;
  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();
    
    double elapsedTime = glfwGetTime();
    if (elapsedTime >= interval)
    {
      updateCells(cells, oldCells, rows, columns);
      memcpy(oldCells, cells, rows * columns * sizeof(AtlrU8));
      glfwSetTime(0.0);
    }

    // begin recording
    if (!atlrBeginFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrBeginFrameCommands returned 0.");
      return -1;
    }
    // begin render pass
    if (!atlrFrameCommandContextBeginRenderPassHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrFrameCommandContextBeginRenderPassHostGLFW returned 0.");
      return -1;
    }

    const VkCommandBuffer commandBuffer = atlrGetFrameCommandContextCommandBufferHostGLFW(&commandContext);
    vkCmdBindPipeline(commandBuffer, pipeline.bindPoint, pipeline.pipeline);
    atlrBindMesh(&quadMesh, commandBuffer);

    for (AtlrU32 i = 0; i < columns; i++)
    {
      transform.translate.x = -1.0f + i * transform.scale.x;
      for (AtlrU32 j = 0; j < rows; j++)
      {
	if (!cells[i * rows + j]) continue;

	transform.translate.y = -1.0f + j * transform.scale.y;
	vkCmdPushConstants(commandBuffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Transform), &transform);
	atlrDrawMesh(&quadMesh, commandBuffer);
      }
    }

    // end render pass
    if (!atlrFrameCommandContextEndRenderPassHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrFrameCommandContextEndRenderPassHostGLFW returned 0.");
      return -1;
    }
    // end recording
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
