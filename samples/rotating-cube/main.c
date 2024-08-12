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
#include <time.h>

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct Vertex
{
  AtlrVec3 pos;
  AtlrVec3 normal;
  
} Vertex;

typedef struct Camera
{
  AtlrVec4 eye;
  AtlrMat4 view;
  AtlrMat4 perspective;
  
} Camera;

typedef struct WorldTransform
{
  AtlrMat4 transform;
  AtlrMat4 normalTransform;
  
} WorldTransform;

typedef struct Pipeline
{
  VkPipelineLayout layout;
  VkPipeline pipeline;
  
} Pipeline;

// projection planes
static const float fov = 45;
static const float nearPlane = 0.1f;
static const float farPlane = 100.0f;

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSwapchain swapchain;
static AtlrSingleRecordCommandContext singleRecordCommandContext;
static AtlrFrameCommandContext commandContext;
static AtlrMesh cubeMesh;
static Camera camera;
static AtlrBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
static AtlrDescriptorSetLayout descriptorSetLayout;
static AtlrDescriptorPool descriptorPool;
static VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
static Pipeline pipeline;

static AtlrU8 initUniformBuffers()
{ 
  const AtlrU64 size = sizeof(Camera);
  const VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    if (!atlrInitBuffer(uniformBuffers + i, size, usage, memoryProperties, &device))
    {
      ATLR_ERROR_MSG("atlrInitBuffer returned 0.");
      return 0;
    }
    if (!atlrMapBuffer(uniformBuffers + i, 0, size, 0))
    {
      ATLR_ERROR_MSG("atlrMapBuffer returned 0.");
      return 0;
    }
  }

  return 1;
}

static void deinitUniformBuffers()
{
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    atlrDeinitBuffer(uniformBuffers + i);
}

static AtlrU8 initDescriptor()
{
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  const VkDescriptorSetLayoutBinding descriptorSetLayoutBinding =
    atlrInitDescriptorSetLayoutBinding(0, type, VK_SHADER_STAGE_VERTEX_BIT);
  if (!atlrInitDescriptorSetLayout(&descriptorSetLayout, 1, &descriptorSetLayoutBinding, &device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorSetLayout returned 0.");
    return 0;
  }

  const VkDescriptorPoolSize poolSize = atlrInitDescriptorPoolSize(type, MAX_FRAMES_IN_FLIGHT);
  if (!atlrInitDescriptorPool(&descriptorPool, MAX_FRAMES_IN_FLIGHT, 1, &poolSize, &device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorPool returned 0.");
    return 0;
  }

  VkDescriptorSetLayout setLayouts[MAX_FRAMES_IN_FLIGHT];
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) setLayouts[i] = descriptorSetLayout.layout;
  if (!atlrAllocDescriptorSets(&descriptorPool, MAX_FRAMES_IN_FLIGHT, setLayouts, descriptorSets))
  {
    ATLR_ERROR_MSG("atlrAllocDescriptorSets returned 0.");
    return 0;
  }

  VkDescriptorBufferInfo bufferInfos[MAX_FRAMES_IN_FLIGHT];
  VkWriteDescriptorSet descriptorWrites[MAX_FRAMES_IN_FLIGHT];
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    bufferInfos[i] = atlrInitDescriptorBufferInfo(uniformBuffers + i, sizeof(Camera));
    descriptorWrites[i] = atlrWriteBufferDescriptorSet(descriptorSets[i], 0, type, bufferInfos + i);
  }
  vkUpdateDescriptorSets(device.logical, MAX_FRAMES_IN_FLIGHT, descriptorWrites, 0, NULL);

  return 1;
}

static void deinitDescriptor()
{
  atlrDeinitDescriptorPool(&descriptorPool);
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    descriptorSets[i] = VK_NULL_HANDLE;
  atlrDeinitDescriptorSetLayout(&descriptorSetLayout);
}

static AtlrU8 initPipeline()
{
  VkShaderModule modules[2] =
  {
    atlrInitShaderModule("cube-vert.spv", &device),
    atlrInitShaderModule("cube-frag.spv", &device)
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
  const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
  {
    (VkVertexInputAttributeDescription)
    {
      .location = 0,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = offsetof(Vertex, pos)
    },
    (VkVertexInputAttributeDescription)
    {
      .location = 1,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = offsetof(Vertex, normal)
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

  const VkPushConstantRange pushConstantRange =
  {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .offset = 0,
    .size = sizeof(WorldTransform)
  };
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    atlrInitPipelineLayoutInfo(1, &descriptorSetLayout.layout, 1, &pushConstantRange);
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
  
static AtlrU8 initRotatingCube()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Rotating Cube' demo ...");

  if (!atlrInitInstanceHostGLFW(&instance, 800, 400, "Rotating Cube Demo"))
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

  if (!atlrInitSingleRecordCommandContext(&singleRecordCommandContext, device.queueFamilyIndices.graphicsComputeIndex, &device))
  {
    ATLR_ERROR_MSG("atlrInitSingleRecordCommandContext returned 0.");
    return 0;
  }

  if (!atlrInitFrameCommandContextHostGLFW(&commandContext, MAX_FRAMES_IN_FLIGHT, &swapchain))
  {
    ATLR_ERROR_MSG("atlrInitFrameCommandContext returned 0.");
    return 0;
  }

  const AtlrVec3 positions[8] =
  {
    {{-0.5f, -0.5f,  0.5f}},
    {{ 0.5f, -0.5f,  0.5f}},
    {{ 0.5f,  0.5f,  0.5f}},
    {{-0.5f,  0.5f,  0.5f}},
    {{-0.5f, -0.5f, -0.5f}},
    {{ 0.5f, -0.5f, -0.5f}},
    {{ 0.5f,  0.5f, -0.5f}},
    {{-0.5f,  0.5f, -0.5f}}
  };
  const AtlrVec3 normals[6] =
  {
    {{ 0.0f,  0.0f,  1.0f}},
    {{ 0.0f,  0.0f, -1.0f}},
    {{ 1.0f,  0.0f,  0.0f}},
    {{-1.0f,  0.0f,  0.0f}},
    {{ 0.0f, -1.0f,  0.0f}},
    {{ 0.0f,  1.0f,  0.0f}}
  };
  const Vertex vertices[24] =
  {
    // top face
    (Vertex){.pos = positions[0], .normal = normals[0]},
    (Vertex){.pos = positions[1], .normal = normals[0]},
    (Vertex){.pos = positions[2], .normal = normals[0]},
    (Vertex){.pos = positions[3], .normal = normals[0]},

    // bottom face
    (Vertex){.pos = positions[7], .normal = normals[1]},
    (Vertex){.pos = positions[6], .normal = normals[1]},
    (Vertex){.pos = positions[5], .normal = normals[1]},
    (Vertex){.pos = positions[4], .normal = normals[1]},

    // right face
    (Vertex){.pos = positions[5], .normal = normals[2]},
    (Vertex){.pos = positions[6], .normal = normals[2]},
    (Vertex){.pos = positions[2], .normal = normals[2]},
    (Vertex){.pos = positions[1], .normal = normals[2]},

    // left face
    (Vertex){.pos = positions[7], .normal = normals[3]},
    (Vertex){.pos = positions[4], .normal = normals[3]},
    (Vertex){.pos = positions[0], .normal = normals[3]},
    (Vertex){.pos = positions[3], .normal = normals[3]},

    // back face
    (Vertex){.pos = positions[4], .normal = normals[4]},
    (Vertex){.pos = positions[5], .normal = normals[4]},
    (Vertex){.pos = positions[1], .normal = normals[4]},
    (Vertex){.pos = positions[0], .normal = normals[4]},

    // front face
    (Vertex){.pos = positions[6], .normal = normals[5]},
    (Vertex){.pos = positions[7], .normal = normals[5]},
    (Vertex){.pos = positions[3], .normal = normals[5]},
    (Vertex){.pos = positions[2], .normal = normals[5]}
  }; 
  const AtlrU16 indices[36] =
  {
    // top face
    0, 2, 1, 3, 2, 0,
    // bottom face
    4, 6, 5, 7, 6, 4,
    // right face
    8, 10, 9, 11, 10, 8,
    // left face
    12, 14, 13, 15, 14, 12,
    // back face
    16, 18, 17, 19, 18, 16,
    // front face
    20, 22, 21, 23, 22, 20
  };
  AtlrU64 verticesSize = 24 * sizeof(Vertex);

  if (!atlrInitMesh(&cubeMesh, verticesSize, vertices, 36, indices, &device, &singleRecordCommandContext))
  {
    ATLR_ERROR_MSG("atlrInitMesh returned 0.");
    return 0;
  }

  if (!initUniformBuffers())
  {
    ATLR_ERROR_MSG("initUniformBuffers returned 0.");
    return 0;
  }

  if (!initDescriptor())
  {
    ATLR_ERROR_MSG("initDescriptor returned 0.");
    return 0;
  }

  if (!initPipeline())
  {
    ATLR_ERROR_MSG("initPipeline returned 0.");
    return 0;
  }

  // camera
  {
    const AtlrVec3 eye = {{8.0f, 0.0f, 4.0f}};
    const AtlrVec3 targetPos = {{0.0f, 0.0f, 0.0f}};
    const AtlrVec3 worldUpDir = {{0.0f, 0.0f, 1.0f}};
    camera.eye = (AtlrVec4){{eye.x, eye.y, eye.z, 0.0f}};
    camera.view = atlrLookAt(&eye, &targetPos, &worldUpDir);
  }
  {
    GLFWwindow* window = instance.data;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    camera.perspective = atlrPerspectiveProjection(fov, (float)height / width, nearPlane, farPlane);
  }

  return 1;
}

static void deinitRotatingCube()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Rotating Cube' demo ...");

  vkDeviceWaitIdle(device.logical);
  
  deinitPipeline();
  deinitDescriptor();
  deinitUniformBuffers();
  atlrDeinitMesh(&cubeMesh);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSingleRecordCommandContext(&singleRecordCommandContext);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
}


int main()
{
  if (!initRotatingCube())
  {
    ATLR_FATAL_MSG("initRotatingCube returned 0.");
    return -1;
  }

  // parameters for the simulation
  float t = 0.0f;
  const float period = 10.0; // period of the cube's oscillatory change in side-length
  const AtlrVec3 axis = {{1.0f, 1.0f, 1.0f}};
  const float v0 = (2 * M_PI) / 8.0f; // initial rotational speed
  float alpha, oldAngle, angle, oldS, s;

  clock_t startTime = clock();
  const double frameTime = 0.016;
  double lag = 0.0;
  GLFWwindow* window = instance.data;
  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    // update
    const double dt = (double)(clock() - startTime) / CLOCKS_PER_SEC;
    for (lag += dt; lag >= frameTime; lag -= frameTime)
    {
      // Relative simulation time t is measured relative to the closest multiple of the period smaller than the real simulation time.
      // The relative angle, called alpha, is updated with this in mind.
      AtlrU32 m = 0;
      for (t += frameTime; t >= period; t -= period) m++;
      if (m) alpha += m * period * v0;
      if (alpha < 0)
	while (alpha < 0) alpha += 2 * M_PI;
      else if (alpha >= 2 * M_PI)
	while (alpha >= 2 * M_PI) alpha -= 2 * M_PI;

      // Update the side-length
      oldS = s;
      s = 1.0f / sqrtf(1.0f + 0.5f * sinf((2 * M_PI * t) / period)); 
      
      // Update the angle.
      oldAngle = angle;
      angle = alpha + (t - (period / (4 * M_PI)) * cosf((2 * M_PI) / period)) * v0;

      // shift the angle of the previous and current update frames in a way that prevents jumps when interpolating.
      if (angle < 0)
      {
	while (angle < 0)
	{
	  oldAngle += 2 * M_PI;
	  angle += 2 * M_PI;
	}
      }
      else if (angle >= 2 * M_PI)
      {
	while (angle >= 2 * M_PI)
	{
	  oldAngle -= 2 * M_PI;
	  angle -= 2 * M_PI;
	}
      }
      
    }
    startTime = clock();

    // start render frame
    if (!atlrBeginFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrBeginFrameCommands returned 0.");
      return -1;
    }
    const VkCommandBuffer commandBuffer = atlrGetFrameCommandContextCommandBufferHostGLFW(&commandContext);

    // world transform data
    const float L = lag / frameTime;
    const AtlrNodeTransform oldNode =
    {
      .scale = (AtlrVec3){{oldS, oldS, oldS}},
      .rotate = atlrUnitQuatFromAxisAngle(&axis, 180 * oldAngle / M_PI),
      .translate = (AtlrVec3){{0.0f, 0.0f, 0.0f}}
    };
    const AtlrNodeTransform node =
    {
      .scale = (AtlrVec3){{s, s, s}},
      .rotate = atlrUnitQuatFromAxisAngle(&axis, 180 * angle / M_PI),
      .translate = (AtlrVec3){{0.0f, 0.0f, 0.0f}}
    };
    const AtlrNodeTransform interpolatedNode = atlrNodeTransformInterpolate(&oldNode, &node, L);
    WorldTransform world;
    world.transform = atlrMat4FromNodeTransform(&interpolatedNode);
    world.normalTransform = atlrMat4NormalFromNodeTransform(&interpolatedNode);

    // update camera
    {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      camera.perspective = atlrPerspectiveProjection(fov, (float)height / width, nearPlane, farPlane);
    }
    memcpy(uniformBuffers[commandContext.currentFrame].data, &camera, sizeof(camera));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, descriptorSets + commandContext.currentFrame, 0, NULL);

    // draw
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    atlrBindMesh(&cubeMesh, commandBuffer);
    vkCmdPushConstants(commandBuffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(WorldTransform), &world);
    atlrDrawMesh(&cubeMesh, commandBuffer);

    // end render frame
    if (!atlrEndFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrEndFrameCommands returned 0.");
      return -1;
    }
  }

  deinitRotatingCube();

  return 0;
}
