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
#include "../../src/camera.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct Vertex
{
  AtlrVec3 pos;
  AtlrVec3 normal;
  
} Vertex;

typedef struct WorldTransform
{
  AtlrMat4 transform;
  AtlrMat4 normalTransform;
  
} WorldTransform;

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSwapchain swapchain;
static AtlrSingleRecordCommandContext singleRecordCommandContext;
static AtlrFrameCommandContext commandContext;
static AtlrMesh cubeMesh;
static AtlrPerspectiveCamera camera;
static AtlrPipeline pipeline;

static AtlrU8 initPipeline()
{
  VkShaderModule modules[2] =
  {
    atlrInitShaderModule("diffuse-vert.spv", &device),
    atlrInitShaderModule("diffuse-frag.spv", &device)
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
  const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
  {
    (VkVertexInputAttributeDescription){.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
    (VkVertexInputAttributeDescription){.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal)}
  };
  const VkPipelineVertexInputStateCreateInfo vertexInputInfo = atlrInitVertexInputStateInfo(1, &vertexInputBindingDescription, 2, vertexInputAttributeDescriptions);

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
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(WorldTransform)
  };
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(1, &camera.descriptorSetLayout.layout, 1, &pushConstantRange);

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

  const VkClearValue clearColor = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}};
  if (!atlrInitSwapchainHostGLFW(&swapchain, 1, NULL, NULL, &clearColor, &device))
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

  if(!atlrInitPerspectiveCameraHostGLFW(&camera, MAX_FRAMES_IN_FLIGHT, 45, 0.1f, 100.0f, &device))
  {
    ATLR_ERROR_MSG("atlrInitPerspectiveCameraHostGLFW returned 0.");
    return 0;
  }
  {
    const AtlrVec3 eyePos = {{8.0f, 0.0f, 4.0f}};
    const AtlrVec3 targetPos = {{0.0f, 0.0f, 0.0f}};
    const AtlrVec3 worldUpDir = {{0.0f, 0.0f, 1.0f}};
    atlrPerspectiveCameraLookAtHostGLFW(&camera, &eyePos, &targetPos, &worldUpDir);
  }

  if (!initPipeline())
  {
    ATLR_ERROR_MSG("initPipeline returned 0.");
    return 0;
  }

  return 1;
}

static void deinitRotatingCube()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Rotating Cube' demo ...");

  vkDeviceWaitIdle(device.logical);
  
  deinitPipeline();
  atlrDeinitPerspectiveCameraHostGLFW(&camera);
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
  const float a = 1.0f;
  const float b = 0.8f;
  const float period = 10.0; // period of the cube's oscillatory change in side-length
  const AtlrVec3 axis = {{1.0f, 1.0f, 1.0f}};
  const float v0 = M_PI; // initial rotational speed
  float alpha, oldAngle, angle, oldS, s;

  GLFWwindow* window = instance.data;
  const double frameTime = 0.016;
  double lag = 0.0;
  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    // update
    const double dt = glfwGetTime();
    glfwSetTime(0.0);
    for (lag += dt; lag >= frameTime; lag -= frameTime)
    {
      // Relative simulation time t is measured relative to the closest multiple of the period smaller than the real simulation time.
      // The relative angle, called alpha, is updated with this in mind.
      AtlrU32 n= 0;
      for (t += frameTime; t >= period; t -= period) n++;
      if (n) alpha += n * period * v0;
      if (alpha < 0)
	while (alpha < 0) alpha += 2 * M_PI;
      else if (alpha >= 2 * M_PI)
	while (alpha >= 2 * M_PI) alpha -= 2 * M_PI;

      // Update the side-length
      oldS = s;
      s = 1.0f / sqrtf(a + b * sinf((2 * M_PI * t) / period)); 
      
      // Update the angle.
      oldAngle = angle;
      angle = alpha + (t - (period * b) / (2 * a * M_PI) * cosf((2 * M_PI * t) / period)) * v0;

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
    atlrUpdatePerspectiveCameraHostGLFW(&camera, commandContext.currentFrame);
    // bind camera descriptor set
    vkCmdBindDescriptorSets(commandBuffer, pipeline.bindPoint, pipeline.layout, 0, 1, camera.descriptorSets + commandContext.currentFrame, 0, NULL);

    // draw
    vkCmdBindPipeline(commandBuffer, pipeline.bindPoint, pipeline.pipeline);
    atlrBindMesh(&cubeMesh, commandBuffer);
    vkCmdPushConstants(commandBuffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(WorldTransform), &world);
    atlrDrawMesh(&cubeMesh, commandBuffer);

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

  deinitRotatingCube();

  return 0;
}
