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
#include <cstdio>
  
#define MAX_FRAMES_IN_FLIGHT 2

typedef struct Vertex
{
  AtlrVec3 pos;
  AtlrVec3 normal;
  AtlrVec2 uv;
  
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
static AtlrMesh planeMesh;
static AtlrMesh sphereMesh;
static AtlrPerspectiveCamera camera;

static AtlrDescriptorPool descriptorPool;

static struct
{
  float extrusion;
  AtlrI32 count;
  AtlrI32 pad[2];
  
} shellUniformData;
static AtlrBuffer shellUniformBuffers[MAX_FRAMES_IN_FLIGHT];
static AtlrDescriptorSetLayout shellDescriptorSetLayout;
static VkDescriptorSet shellDescriptorSets[MAX_FRAMES_IN_FLIGHT];

static struct
{
  AtlrI32 resolution;
  float thickness;
  float occlusionAttenuation;
  float diffuseContrib;
  
} grassUniformData;
static AtlrBuffer grassUniformBuffers[MAX_FRAMES_IN_FLIGHT];
static AtlrDescriptorSetLayout grassDescriptorSetLayout;
static VkDescriptorSet grassDescriptorSets[MAX_FRAMES_IN_FLIGHT];

static AtlrPipeline grassPipeline;
static Atlr::ImguiContext imguiContext;

static AtlrU8 initDescriptor()
{
  const VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    if (!atlrInitBuffer(shellUniformBuffers + i, sizeof(shellUniformData), usage, memoryProperties, &device))
    {
      ATLR_ERROR_MSG("atlrInitBuffer returned 0.");
      return 0;
    }
    if (!atlrMapBuffer(shellUniformBuffers + i, 0, sizeof(shellUniformData), 0))
    {
      ATLR_ERROR_MSG("atlrMapBuffer returned 0.");
      return 0;
    }
#ifdef ATLR_DEBUG
    char shellBufferString[64];
    sprintf(shellBufferString, "Shell Uniform Buffer ; Frame %d", i);
    atlrSetBufferName(shellUniformBuffers + i, shellBufferString);
#endif

    if (!atlrInitBuffer(grassUniformBuffers + i, sizeof(grassUniformData), usage, memoryProperties, &device))
    {
      ATLR_ERROR_MSG("atlrInitBuffer returned 0.");
      return 0;
    }
    if (!atlrMapBuffer(grassUniformBuffers + i, 0, sizeof(grassUniformData), 0))
    {
      ATLR_ERROR_MSG("atlrMapBuffer returned 0.");
      return 0;
    }
#ifdef ATLR_DEBUG
    char grassBufferString[64];
    sprintf(grassBufferString, "Grass Uniform Buffer ; Frame %d", i);
    atlrSetBufferName(grassUniformBuffers + i, grassBufferString);
#endif
  }

  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  
  VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = atlrInitDescriptorSetLayoutBinding(0, type, VK_SHADER_STAGE_GEOMETRY_BIT);
  if (!atlrInitDescriptorSetLayout(&shellDescriptorSetLayout, 1, &descriptorSetLayoutBinding, &device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorSetLayout returned 0.");
    return 0;
  }
  descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  if (!atlrInitDescriptorSetLayout(&grassDescriptorSetLayout, 1, &descriptorSetLayoutBinding, &device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorSetLayout returned 0.");
    return 0;
  }

  VkDescriptorPoolSize poolSize = atlrInitDescriptorPoolSize(type, 2 * MAX_FRAMES_IN_FLIGHT);
  if (!atlrInitDescriptorPool(&descriptorPool, 2 * MAX_FRAMES_IN_FLIGHT, 1, &poolSize, &device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorPool returned 0.");
    return 0;
  }

  VkDescriptorSetLayout setLayouts[2 * MAX_FRAMES_IN_FLIGHT];
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    setLayouts[i]                        = shellDescriptorSetLayout.layout;
    setLayouts[MAX_FRAMES_IN_FLIGHT + i] = grassDescriptorSetLayout.layout;
  }
  if (!atlrAllocDescriptorSets(&descriptorPool, MAX_FRAMES_IN_FLIGHT, setLayouts, shellDescriptorSets))
  {
    ATLR_ERROR_MSG("atlrAllocDescriptorSets returned 0.");
    return 0;
  }
  if (!atlrAllocDescriptorSets(&descriptorPool, MAX_FRAMES_IN_FLIGHT, setLayouts + MAX_FRAMES_IN_FLIGHT, grassDescriptorSets))
  {
    ATLR_ERROR_MSG("atlrAllocDescriptorSets returned 0.");
    return 0;
  }

  VkDescriptorBufferInfo bufferInfos[2 * MAX_FRAMES_IN_FLIGHT];
  VkWriteDescriptorSet descriptorWrites[2 * MAX_FRAMES_IN_FLIGHT];
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    bufferInfos[i] = atlrInitDescriptorBufferInfo(shellUniformBuffers + i, sizeof(shellUniformData));
    descriptorWrites[i] = atlrWriteBufferDescriptorSet(shellDescriptorSets[i], 0, type, bufferInfos + i);

    bufferInfos[MAX_FRAMES_IN_FLIGHT + i] = atlrInitDescriptorBufferInfo(grassUniformBuffers + i, sizeof(grassUniformData));
    descriptorWrites[MAX_FRAMES_IN_FLIGHT + i] = atlrWriteBufferDescriptorSet(grassDescriptorSets[i], 0, type, bufferInfos + MAX_FRAMES_IN_FLIGHT + i);
  }
  vkUpdateDescriptorSets(device.logical, 2 * MAX_FRAMES_IN_FLIGHT, descriptorWrites, 0, NULL);

  return 1;
}

static void deinitDescriptor()
{
  atlrDeinitDescriptorPool(&descriptorPool);
  atlrDeinitDescriptorSetLayout(&grassDescriptorSetLayout);
  atlrDeinitDescriptorSetLayout(&shellDescriptorSetLayout);
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    AtlrBuffer* shellBuffer = shellUniformBuffers + i;
    atlrUnmapBuffer(shellBuffer);
    atlrDeinitBuffer(shellBuffer);

    AtlrBuffer* grassBuffer = grassUniformBuffers + i;
    atlrUnmapBuffer(grassBuffer);
    atlrDeinitBuffer(grassBuffer);
  }
}

static AtlrU8 initPipelines()
{
  VkShaderModule vertexModule = atlrInitShaderModule("shell-vert.spv", &device);
  VkPipelineShaderStageCreateInfo vertexStage = atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexModule);
  VkShaderModule geometryModule = atlrInitShaderModule("shell-geom.spv", &device);
  VkPipelineShaderStageCreateInfo geometryStage = atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_GEOMETRY_BIT, geometryModule);

  const VkVertexInputBindingDescription vertexInputBindingDescription =
  {
    .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };
  const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[3] =
  {
    (VkVertexInputAttributeDescription){.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
    (VkVertexInputAttributeDescription){.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal)},
    (VkVertexInputAttributeDescription){.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv)}
  };
  const VkPipelineVertexInputStateCreateInfo vertexInputInfo = atlrInitVertexInputStateInfo(1, &vertexInputBindingDescription, 3, vertexInputAttributeDescriptions);

  const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = atlrInitPipelineInputAssemblyStateInfo();
  const VkPipelineViewportStateCreateInfo viewportInfo           = atlrInitPipelineViewportStateInfo();
  const VkPipelineRasterizationStateCreateInfo rasterizationInfo = atlrInitPipelineRasterizationStateInfo();
  const VkPipelineMultisampleStateCreateInfo multisampleInfo     = atlrInitPipelineMultisampleStateInfo(device.msaaSamples);
  const VkPipelineDepthStencilStateCreateInfo depthStencilInfo   = atlrInitPipelineDepthStencilStateInfo();
  const VkPipelineColorBlendAttachmentState colorBlendAttachment = atlrInitPipelineColorBlendAttachmentStateAlpha();
  const VkPipelineColorBlendStateCreateInfo colorBlendInfo       = atlrInitPipelineColorBlendStateInfo(&colorBlendAttachment);
  const VkPipelineDynamicStateCreateInfo dynamicInfo             = atlrInitPipelineDynamicStateInfo();

  const VkDescriptorSetLayout setLayouts[3] = {camera.descriptorSetLayout.layout, shellDescriptorSetLayout.layout, grassDescriptorSetLayout.layout};
  const VkPushConstantRange pushConstantRange =
  {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(WorldTransform)
  };
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(3, setLayouts, 1, &pushConstantRange);

  // grass pipeline
  {
    VkShaderModule fragmentModule = atlrInitShaderModule("grass-frag.spv", &device);
    VkPipelineShaderStageCreateInfo stageInfos[3] = {vertexStage, geometryStage, atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentModule)};
    if(!atlrInitGraphicsPipeline(&grassPipeline,
				 3, stageInfos, &vertexInputInfo, &inputAssemblyInfo, NULL, &viewportInfo, &rasterizationInfo, &multisampleInfo, &depthStencilInfo, &colorBlendInfo, &dynamicInfo, &pipelineLayoutInfo,
				 &device, &swapchain.renderPass))
    {
      ATLR_ERROR_MSG("atlrInitGraphicsPipeline returned 0.");
      return 0;
    }
    atlrDeinitShaderModule(fragmentModule, &device);
  }

  atlrDeinitShaderModule(vertexModule, &device);
  atlrDeinitShaderModule(geometryModule, &device);
  
  return 1;
}

static void deinitPipelines()
{
  atlrDeinitPipeline(&grassPipeline);
}
  
static AtlrU8 initShellTexturing()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Shell Texturing' demo ...");

  if (!atlrInitInstanceHostGLFW(&instance, 800, 400, "Shell Texturing Demo"))
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
			 ATLR_DEVICE_CRITERION_GEOMETRY_SHADER,
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

  // plane mesh
  {
    const AtlrVec3 positions[4] =
    {
      {{-0.5f, -0.5f,  0.0f}},
      {{ 0.5f, -0.5f,  0.0f}},
      {{ 0.5f,  0.5f,  0.0f}},
      {{-0.5f,  0.5f,  0.0f}}
    };
    const AtlrVec3 normal = {{0.0f, 0.0f, 1.0f}};
    const Vertex vertices[4] =
    {
      // top face
      (Vertex){.pos = positions[0], .normal = normal, .uv = {{0.0f, 0.0f}}},
      (Vertex){.pos = positions[1], .normal = normal, .uv = {{1.0f, 0.0f}}},
      (Vertex){.pos = positions[2], .normal = normal, .uv = {{1.0f, 1.0f}}},
      (Vertex){.pos = positions[3], .normal = normal, .uv = {{0.0f, 1.0f}}}
    }; 
    const AtlrU16 indices[6] = {0, 2, 1, 3, 2, 0};
    
    if (!atlrInitMesh(&planeMesh, 4 * sizeof(Vertex), vertices, 6, indices, &device, &singleRecordCommandContext))
    {
      ATLR_ERROR_MSG("atlrInitMesh returned 0.");
      return 0;
    }
#ifdef ATLR_DEBUG
    atlrSetMeshName(&planeMesh, "Plane");
#endif
  }
  
  // sphere mesh
  {
    // Generating a unit sphere the easy way (no nice-looking icosphere, haha).
    // x = sin(u) cos(v), y = sin(u) sin(v), z = cos(u)
    const AtlrU8 polarCount = 16; // number of latitude lines
    const AtlrU8 azimuthalCount = 16; // number of longitude lines, MUST be a power of 2!
    const float du = M_PI / (polarCount - 1); 
    const float dv = 2.0f * M_PI / azimuthalCount;

    const AtlrU64 d = polarCount * azimuthalCount;
    Vertex vertices[d];
    AtlrVec3 r;
    AtlrVec2 st;
    for (AtlrU8 i = 0; i < polarCount; i++)
    {
      const float u = i * du;
      const float rho = 0.25f * sinf(u);
      r.z = 0.25f * cosf(u);
      
      st.y = i / (float)(polarCount - 1);
      
      for (AtlrU8 j = 0; j < azimuthalCount; j++)
      {
	const float v = j * dv;
	r.x = rho * cosf(v);
	r.y = rho * sinf(v);
	
	st.x = (j <= (azimuthalCount - 1) / 2) ? j / (float)(azimuthalCount - 1) : (azimuthalCount - j) / (float)(azimuthalCount - 1);
	
	vertices[i * azimuthalCount + j] = (Vertex){.pos = r, .normal = atlrVec3Normalize(&r), .uv = st};
      }
    }

    const AtlrU64 e =  6 * azimuthalCount * (polarCount - 1);
    AtlrU16 indices[e];
    for (AtlrU8 i = 1; i < polarCount; i++)
    {
      const AtlrU64 um = (i - 1) * azimuthalCount;
      const AtlrU64 up = i * azimuthalCount;
      for (AtlrU8 j = 0; j < azimuthalCount; j++)
      {
	const AtlrU64 v = (j + 1) & (azimuthalCount - 1);
	const AtlrU64 idx = (um + j) * 6;
      
	indices[idx]     = um + j;
	indices[idx + 2] = up + j; 
	indices[idx + 1] = um + v;
	indices[idx + 3] = um + v;
	indices[idx + 5] = up + j;
	indices[idx + 4] = up + v;
      }
    }

    if (!atlrInitMesh(&sphereMesh, sizeof(vertices), vertices, sizeof(indices) / sizeof(AtlrU16), indices, &device, &singleRecordCommandContext))
    {
      ATLR_ERROR_MSG("atlrInitMesh returned 0.");
      return 0;
    }
#ifdef ATLR_DEBUG
    atlrSetMeshName(&sphereMesh, "Sphere");
#endif
  }

  if(!atlrInitPerspectiveCameraHostGLFW(&camera, MAX_FRAMES_IN_FLIGHT, 45, 0.1f, 100.0f, &device))
  {
    ATLR_ERROR_MSG("atlrInitPerspectiveCameraHostGLFW returned 0.");
    return 0;
  }
  {
    const AtlrVec3 eyePos = {{2.0f, 0.0f, 1.0f}};
    const AtlrVec3 targetPos = {{0.0f, 0.0f, 0.0f}};
    const AtlrVec3 worldUpDir = {{0.0f, 0.0f, 1.0f}};
    atlrPerspectiveCameraLookAtHostGLFW(&camera, &eyePos, &targetPos, &worldUpDir);
  }

  if(!initDescriptor())
  {
    ATLR_ERROR_MSG("initDescriptor returned 0.");
    return 0;
  }

  if (!initPipelines())
  {
    ATLR_ERROR_MSG("initPipeline returned 0.");
    return 0;
  }

  imguiContext.init(MAX_FRAMES_IN_FLIGHT, &swapchain, &singleRecordCommandContext);

  return 1;
}

static void deinitShellTexturing()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Shell Texturing' demo ...");

  vkDeviceWaitIdle(device.logical);

  imguiContext.deinit();
  deinitPipelines();
  deinitDescriptor();
  atlrDeinitPerspectiveCameraHostGLFW(&camera);
  atlrDeinitMesh(&sphereMesh);
  atlrDeinitMesh(&planeMesh);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSingleRecordCommandContext(&singleRecordCommandContext);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
}

int main()
{
  if (!initShellTexturing())
  {
    ATLR_FATAL_MSG("initShellTexturing returned 0.");
    return -1;
  }

  AtlrVec3 axis = (AtlrVec3){{0.0f, 0.0f, 1.0f}};
  float angle = 0.0;
  AtlrNodeTransform node = 
  {
      .scale = (AtlrVec3){{1.0f, 1.0f, 1.0f}},
      .rotate = atlrUnitQuatFromAxisAngle(&axis, angle),
      .translate = (AtlrVec3){{0.0f, 0.0f, -0.5f}}
  };

  // geometry shader initial shell uniform data
  shellUniformData.extrusion = 0.05f;
  shellUniformData.count = 16;

  // fragment shader initial grass uniform data
  grassUniformData.resolution = 64;
  grassUniformData.thickness = 0.85f;
  grassUniformData.occlusionAttenuation = 2.5f;
  grassUniformData.diffuseContrib = 0.2f;

  // toggle between sphere and plane
  enum MeshType {MESH_TYPE_PLANE, MESH_TYPE_SPHERE};
  int meshType = MESH_TYPE_PLANE;

  GLFWwindow* window = (GLFWwindow*)instance.data;
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
    WorldTransform world;
    node.rotate = atlrUnitQuatFromAxisAngle(&axis, angle);
    world.transform = atlrMat4FromNodeTransform(&node);
    world.normalTransform = atlrMat4NormalFromNodeTransform(&node);

    AtlrPipeline* pipeline = &grassPipeline;
    AtlrMesh* msh = NULL;
    switch (meshType)
    {
     case MESH_TYPE_PLANE: msh = &planeMesh; break;
     case MESH_TYPE_SPHERE: msh = &sphereMesh; break;
    }

    // update camera
    atlrUpdatePerspectiveCameraHostGLFW(&camera, commandContext.currentFrame);
    // bind camera descriptor set
    vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout, 0, 1, camera.descriptorSets + commandContext.currentFrame, 0, NULL);

#ifdef ATLR_DEBUG
    {
      const float color[4] = {0.2f, 0.2f, 0.9f, 1.0f};
      atlrBeginCommandLabel(commandBuffer, "Shells draw", color, &instance);
    }
#endif

    // update shell data
    memcpy(shellUniformBuffers[commandContext.currentFrame].data, &shellUniformData, sizeof(shellUniformData));
    // bind shell descriptor set
    vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout, 1, 1, shellDescriptorSets + commandContext.currentFrame, 0, NULL);

    // update grass data
    memcpy(grassUniformBuffers[commandContext.currentFrame].data, &grassUniformData, sizeof(grassUniformData));
    // bind grass descriptor set
    vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout, 2, 1, grassDescriptorSets + commandContext.currentFrame, 0, NULL);

    // draw scene
    vkCmdBindPipeline(commandBuffer, pipeline->bindPoint, pipeline->pipeline);
    atlrBindMesh(msh, commandBuffer);
    vkCmdPushConstants(commandBuffer, pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(WorldTransform), &world);
    atlrDrawMesh(msh, commandBuffer);

#ifdef ATLR_DEBUG
    atlrEndCommandLabel(commandBuffer, &instance);
#endif

    // imgui
    imguiContext.bind(commandBuffer, commandContext.currentFrame);
    ImGui::Begin("Shell Texturing Widget");
    ImGui::RadioButton("plane mesh", &meshType, MESH_TYPE_PLANE); ImGui::SameLine();
    ImGui::RadioButton("sphere mesh", &meshType, MESH_TYPE_SPHERE);
    if (ImGui::CollapsingHeader("Mesh Transform"))
    {
      ImGui::SliderFloat("scale.x", &node.scale.x, 1.0f, 5.0f, "%.3f", 0);
      ImGui::SliderFloat("scale.y", &node.scale.y, 1.0f, 5.0f, "%.3f", 0);
      ImGui::SliderFloat("scale.z", &node.scale.z, 1.0f, 5.0f, "%.3f", 0);
      ImGui::SliderFloat("rotate-axis.x", &axis.x, 0.0f, 1.0f, "%.3f", 0);
      ImGui::SliderFloat("rotate-axis.y", &axis.y, 0.0f, 1.0f, "%.3f", 0);
      ImGui::SliderFloat("rotate-axis.z", &axis.z, 0.0f, 1.0f, "%.3f", 0);
      ImGui::SliderFloat("rotate-angle", &angle, -180.0f, 180.0f, "%.3f", 0);
      ImGui::SliderFloat("translate.x", &node.translate.x, -1.0f, 1.0f, "%.3f", 0);
      ImGui::SliderFloat("translate.y", &node.translate.y, -1.0f, 1.0f, "%.3f", 0);
      ImGui::SliderFloat("translate.z", &node.translate.z, -1.0f, 1.0f, "%.3f", 0);
    }
    if (ImGui::CollapsingHeader("Shell Data"))
    {
      ImGui::SliderFloat("total extrusion", &shellUniformData.extrusion, 0.01f, 0.2f, 0);
      ImGui::SliderInt("shell count", &shellUniformData.count, 1, 64);
    }
    if (ImGui::CollapsingHeader("Grass Data"))
    {
      ImGui::SliderInt("resolution", &grassUniformData.resolution, 8, 256);
      ImGui::SliderFloat("thickness", &grassUniformData.thickness, 0.2f, 3.0f, "%.3f", 0);
      ImGui::SliderFloat("occlusion attenuation", &grassUniformData.occlusionAttenuation, 0.0f, 5.0f, "%.3f", 0);
      ImGui::SliderFloat("diffuse contribution", &grassUniformData.diffuseContrib, 0.0f, 1.0f, "%.3f", 0);
    }
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

  deinitShellTexturing();

  return 0;
}
