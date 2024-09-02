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

#include "../../src/cpp-compat.hpp"
extern "C"
{ 
#include "../../src/antler.h"
#include "../../src/transforms.h"
#include "../../src/camera.h"
}
#include "../../lib/imgui/imgui.h"
#include "../../src/antler-imgui.hpp"
  
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
static Atlr::ImguiContext imguiContext;

static AtlrU8 initPipeline()
{
  VkShaderModule modules[2] =
  {
    atlrInitShaderModule("diffuse-vert.spv", &device),
    atlrInitShaderModule("diffuse-frag.spv", &device)
  };
  VkPipelineShaderStageCreateInfo stageInfos[2] =
  {
    atlrInitPipelineVertexShaderStageInfo(modules[0]),
    atlrInitPipelineFragmentShaderStageInfo(modules[1])
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
  
static AtlrU8 initTransformCube()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Transform Cube' demo ...");

  if (!glslang_initialize_process())
  {
    ATLR_ERROR_MSG("glslang_initialize_process returned 0.");
    return 0;
  }

  if (!atlrInitInstanceHostGLFW(&instance, 800, 400, "Transform Cube Demo"))
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

  imguiContext.init(MAX_FRAMES_IN_FLIGHT, &swapchain, &singleRecordCommandContext);

  return 1;
}

static void deinitTransformCube()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Transform Cube' demo ...");

  vkDeviceWaitIdle(device.logical);

  imguiContext.deinit();
  deinitPipeline();
  atlrDeinitPerspectiveCameraHostGLFW(&camera);
  atlrDeinitMesh(&cubeMesh);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSingleRecordCommandContext(&singleRecordCommandContext);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
  glslang_finalize_process();
}

int main()
{
  if (!initTransformCube())
  {
    ATLR_FATAL_MSG("initTransformCube returned 0.");
    return -1;
  }

  AtlrVec3 axis = (AtlrVec3){{0.0f, 0.0f, 1.0f}};
  float angle = 0.0;
  AtlrNodeTransform node = 
  {
      .scale = (AtlrVec3){{1.0f, 1.0f, 1.0f}},
      .rotate = atlrUnitQuatFromAxisAngle(&axis, angle),
      .translate = (AtlrVec3){{0.0f, 0.0f, 0.0f}}
  };

  GLFWwindow* window = (GLFWwindow*)instance.data;
  const double frameTime = 0.016;
  double lag = 0.0;
  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

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
    WorldTransform world;
    node.rotate = atlrUnitQuatFromAxisAngle(&axis, angle);
    world.transform = atlrMat4FromNodeTransform(&node);
    world.normalTransform = atlrMat4NormalFromNodeTransform(&node);

    // update camera
    atlrUpdatePerspectiveCameraHostGLFW(&camera, commandContext.currentFrame);
    // bind camera descriptor set
    vkCmdBindDescriptorSets(commandBuffer, pipeline.bindPoint, pipeline.layout, 0, 1, camera.descriptorSets + commandContext.currentFrame, 0, NULL);

    // draw scene
    vkCmdBindPipeline(commandBuffer, pipeline.bindPoint, pipeline.pipeline);
    atlrBindMesh(&cubeMesh, commandBuffer);
    vkCmdPushConstants(commandBuffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(WorldTransform), &world);
    atlrDrawMesh(&cubeMesh, commandBuffer);

    // imgui
    imguiContext.bind(commandBuffer, commandContext.currentFrame);
    ImGui::Begin("Transform Widget");
    ImGui::SliderFloat("scale.x", &node.scale.x, 1.0f, 5.0f, "%.3f", 0);
    ImGui::SliderFloat("scale.y", &node.scale.y, 1.0f, 5.0f, "%.3f", 0);
    ImGui::SliderFloat("scale.z", &node.scale.z, 1.0f, 5.0f, "%.3f", 0);
    ImGui::SliderFloat("rotate-axis.x", &axis.x, 0.0f, 1.0f, "%.3f", 0);
    ImGui::SliderFloat("rotate-axis.y", &axis.y, 0.0f, 1.0f, "%.3f", 0);
    ImGui::SliderFloat("rotate-axis.z", &axis.z, 0.0f, 1.0f, "%.3f", 0);
    ImGui::SliderFloat("rotate-angle", &angle, 0.0f, 360.0f, "%.3f", 0);
    ImGui::SliderFloat("translate.x", &node.translate.x, -1.0f, 1.0f, "%.3f", 0);
    ImGui::SliderFloat("translate.y", &node.translate.y, -1.0f, 1.0f, "%.3f", 0);
    ImGui::SliderFloat("translate.z", &node.translate.z, -1.0f, 1.0f, "%.3f", 0);
    ImGui::End();
    imguiContext.draw(commandBuffer, commandContext.currentFrame);

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

  deinitTransformCube();

  return 0;
}
