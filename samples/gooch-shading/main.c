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
#include "../../src/offscreen-canvas.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct Vertex
{
  AtlrVec3 pos;
  AtlrVec3 normal;
  
} Vertex;

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSwapchain swapchain;
static AtlrOffscreenCanvas offscreenCanvas;
static AtlrSingleRecordCommandContext singleRecordCommandContext;
static AtlrFrameCommandContext commandContext;
static AtlrMesh sphereMesh;
static AtlrBuffer edgeDetectIndexBuffer;
static AtlrPerspectiveCamera camera;

static VkSampler sampler;
static AtlrDescriptorSetLayout descriptorSetLayout;
static AtlrDescriptorPool descriptorPool;
static VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
static AtlrPipeline goochPipeline;
static AtlrPipeline edgeDetectPipeline;

static AtlrU8 onReinitSwapchain(void* data)
{
  atlrDeinitOffscreenCanvas(&offscreenCanvas, 0);
  if (!atlrInitOffscreenCanvas(&offscreenCanvas, &swapchain.extent, swapchain.format, 0, NULL, &device))
  {
    ATLR_ERROR_MSG("atlrInitOffscreenCanvas returned 0.");
    return 0;
  }

  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  const VkDescriptorImageInfo imageInfo = atlrInitDescriptorImageInfo(&offscreenCanvas.colorImage, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  const VkDescriptorImageInfo depthInfo = atlrInitDescriptorImageInfo(&offscreenCanvas.depthImage, sampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
  VkWriteDescriptorSet descriptorWrites[2 * MAX_FRAMES_IN_FLIGHT];
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    descriptorWrites[2 * i]     = atlrWriteImageDescriptorSet(descriptorSets[i], 0, type, &imageInfo);
    descriptorWrites[2 * i + 1] = atlrWriteImageDescriptorSet(descriptorSets[i], 1, type, &depthInfo);
  };
  vkUpdateDescriptorSets(device.logical, 2 * MAX_FRAMES_IN_FLIGHT, descriptorWrites, 0, NULL);

  return 1;
}

static AtlrU8 initDescriptor()
{
  // sampler
  VkSamplerCreateInfo samplerInfo =
  {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_FALSE,
    .maxAnisotropy = 1.0f,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .minLod =  0.0f,
    .maxLod = 0.0f,
    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    .unnormalizedCoordinates = VK_FALSE
  };
  if (vkCreateSampler(device.logical, &samplerInfo, instance.allocator, &sampler) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateSampler did not return VK_SUCCESS.");
    return 0;
  }
  
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  
  const VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] =
  {
    atlrInitDescriptorSetLayoutBinding(0, type, VK_SHADER_STAGE_FRAGMENT_BIT),
    atlrInitDescriptorSetLayoutBinding(1, type, VK_SHADER_STAGE_FRAGMENT_BIT)
  };
  if (!atlrInitDescriptorSetLayout(&descriptorSetLayout, 2, descriptorSetLayoutBindings, &device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorSetLayout returned 0.");
    return 0;
  }

  VkDescriptorPoolSize poolSize = atlrInitDescriptorPoolSize(type, MAX_FRAMES_IN_FLIGHT);
  VkDescriptorPoolSize poolSizes[2]; poolSizes[0] = poolSize; poolSizes[1] = poolSize;
  if (!atlrInitDescriptorPool(&descriptorPool, MAX_FRAMES_IN_FLIGHT, 2, poolSizes, &device))
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

  const VkDescriptorImageInfo imageInfo = atlrInitDescriptorImageInfo(&offscreenCanvas.colorImage, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  const VkDescriptorImageInfo depthInfo = atlrInitDescriptorImageInfo(&offscreenCanvas.depthImage, sampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
  VkWriteDescriptorSet descriptorWrites[2 * MAX_FRAMES_IN_FLIGHT];
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    descriptorWrites[2 * i]     = atlrWriteImageDescriptorSet(descriptorSets[i], 0, type, &imageInfo);
    descriptorWrites[2 * i + 1] = atlrWriteImageDescriptorSet(descriptorSets[i], 1, type, &depthInfo);
  };
  vkUpdateDescriptorSets(device.logical, 2 * MAX_FRAMES_IN_FLIGHT, descriptorWrites, 0, NULL);

  return 1;
}

static void deinitDescriptor()
{
  vkDestroySampler(device.logical, sampler, instance.allocator);
  atlrDeinitDescriptorPool(&descriptorPool);
  atlrDeinitDescriptorSetLayout(&descriptorSetLayout);
}

static AtlrU8 initPipelines()
{
  const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = atlrInitPipelineInputAssemblyStateInfo();
  const VkPipelineViewportStateCreateInfo viewportInfo           = atlrInitPipelineViewportStateInfo();
  const VkPipelineRasterizationStateCreateInfo rasterizationInfo = atlrInitPipelineRasterizationStateInfo();
  const VkPipelineMultisampleStateCreateInfo multisampleInfo     = atlrInitPipelineMultisampleStateInfo(VK_SAMPLE_COUNT_1_BIT);
  const VkPipelineMultisampleStateCreateInfo msaaMultisampleInfo = atlrInitPipelineMultisampleStateInfo(device.msaaSamples);
  const VkPipelineDepthStencilStateCreateInfo depthStencilInfo   = atlrInitPipelineDepthStencilStateInfo();
  const VkPipelineDynamicStateCreateInfo dynamicInfo             = atlrInitPipelineDynamicStateInfo();
  
  const VkPipelineColorBlendAttachmentState alphaBlend           = atlrInitPipelineColorBlendAttachmentStateAlpha();
  VkPipelineColorBlendAttachmentState opaqueBlend                = alphaBlend;
  opaqueBlend.blendEnable = VK_FALSE;
  
  const VkPipelineColorBlendStateCreateInfo alphaBlendInfo       = atlrInitPipelineColorBlendStateInfo(&alphaBlend);
  const VkPipelineColorBlendStateCreateInfo opaqueBlendInfo      = atlrInitPipelineColorBlendStateInfo(&opaqueBlend);

  // gooch lighting pipeline 
  {
    const VkShaderModule modules[2]                     = { atlrInitShaderModule("gooch-vert.spv", &device), atlrInitShaderModule("gooch-frag.spv", &device) };
    const VkPipelineShaderStageCreateInfo stageInfos[2] = { atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, modules[0]), atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, modules[1]) };

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

    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(1, &camera.descriptorSetLayout.layout, 0, NULL);

    if(!atlrInitGraphicsPipeline(&goochPipeline,
				 2, stageInfos, &vertexInputInfo, &inputAssemblyInfo, NULL, &viewportInfo, &rasterizationInfo, &multisampleInfo, &depthStencilInfo, &alphaBlendInfo, &dynamicInfo, &pipelineLayoutInfo,
				 &device, &offscreenCanvas.renderPass))
    {
      ATLR_ERROR_MSG("atlrInitGraphicsPipeline returned 0.");
      return 0;
    }

    atlrDeinitShaderModule(modules[0], &device);
    atlrDeinitShaderModule(modules[1], &device);
  }

  // edge detection pipeline
  {
    const VkShaderModule modules[2]                     = { atlrInitShaderModule("edge-vert.spv", &device), atlrInitShaderModule("edge-frag.spv", &device) };
    const VkPipelineShaderStageCreateInfo stageInfos[2] = { atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, modules[0]), atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, modules[1]) };
    
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = atlrInitVertexInputStateInfo(0, NULL, 0, NULL);

    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(1, &descriptorSetLayout.layout, 0, NULL);

    if(!atlrInitGraphicsPipeline(&edgeDetectPipeline,
				 2, stageInfos, &vertexInputInfo, &inputAssemblyInfo, NULL, &viewportInfo, &rasterizationInfo, &msaaMultisampleInfo, &depthStencilInfo, &opaqueBlendInfo, &dynamicInfo, &pipelineLayoutInfo,
				 &device, &swapchain.renderPass))
    {
      ATLR_ERROR_MSG("atlrInitGraphicsPipeline returned 0.");
      return 0;
    }

    atlrDeinitShaderModule(modules[0], &device);
    atlrDeinitShaderModule(modules[1], &device);
  }
  
  return 1;
}

static void deinitPipelines()
{
  atlrDeinitPipeline(&edgeDetectPipeline);
  atlrDeinitPipeline(&goochPipeline);
}
  
static AtlrU8 initGooch()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Gooch' demo ...");

  if (!atlrInitInstanceHostGLFW(&instance, 800, 400, "Gooch Demo"))
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

  const VkClearValue clearColor = {.color = {.float32 = {1.0f, 1.0f, 1.0f, 1.0f}}};
  if (!atlrInitSwapchainHostGLFW(&swapchain, 1, onReinitSwapchain, NULL, &clearColor, &device))
  {
    ATLR_ERROR_MSG("atlrInitSwapchainHostGLFW returned 0.");
    return 0;
  }

  if (!atlrInitOffscreenCanvas(&offscreenCanvas, &swapchain.extent, swapchain.format, 1, &clearColor, &device))
  {
    ATLR_ERROR_MSG("atlrInitOffscreenCanvas returned 0.");
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

  // Generating a unit sphere the easy way (no nice-looking icosphere, haha).
  // x = sin(u) cos(v), y = sin(u) sin(v), z = cos(u)
  // Sphere is a manifold that needs min of two charts, so the poles separately
  // 0 = f(x,y,z) = x^2 + y^2 + z^2 - 1, normal = grad(f) / |grad(f)| = (x y z)
  const AtlrU8 polarCount = 16; // number of latitude lines
  const AtlrU8 azimuthalCount = 16; // number of longitude lines, MUST be a power of 2!
  const float du = M_PI / (polarCount - 1);
  const float dv = 2.0f * M_PI / azimuthalCount;

  const AtlrU64 d = (polarCount - 2) * azimuthalCount;
  Vertex vertices[d + 2];
  AtlrVec3 r;
  for (AtlrU8 i = 1; i < polarCount - 1; i++)
  {
    const float u = i * du;
    const float rho = sinf(u);
    r.z = cosf(u);
    for (AtlrU8 j = 0; j < azimuthalCount; j++)
    {
      const float v = j * dv;
      r.x = rho * cosf(v);
      r.y = rho * sinf(v);
      
      vertices[(i - 1) * azimuthalCount + j] = (Vertex){.pos = r, .normal = r};
    }
  }
  vertices[d]     = (Vertex){.pos = {{0.0f, 0.0f,  1.0f}}, .normal = {{0.0f, 0.0f,  1.0f}}};
  vertices[d + 1] = (Vertex){.pos = {{0.0f, 0.0f, -1.0f}}, .normal = {{0.0f, 0.0f, -1.0f}}};

  const AtlrU64 e =  6 * azimuthalCount * (polarCount - 3);
  AtlrU16 indices[e + 2 * 3 * azimuthalCount];
  for (AtlrU8 i = 1; i < polarCount - 1; i++)
  {
    const AtlrU64 um = (i - 1) * azimuthalCount;
    const AtlrU64 up = i * azimuthalCount;
    for (AtlrU8 j = 0; j < azimuthalCount; j++)
    {
      const AtlrU64 v = (j + 1) & (azimuthalCount - 1);
      const AtlrU64 idx = (um + j) * 6;
      
      indices[idx]     = um + j;
      indices[idx + 1] = up + j; 
      indices[idx + 2] = um + v;
      indices[idx + 3] = um + v;
      indices[idx + 4] = up + j;
      indices[idx + 5] = up + v;
    }
  }
  for (AtlrU8 j = 0; j < azimuthalCount; j++)
  {
    const AtlrU64 idx = e + j * 3;  
    indices[idx]     = d;
    indices[idx + 1] = j; 
    indices[idx + 2] = ((j + 1) & (azimuthalCount - 1));
  }
  for (AtlrU8 j = 0; j < azimuthalCount; j++)
  {
    const AtlrU64 idx = e + (j + azimuthalCount) * 3;  
    indices[idx]     = d - azimuthalCount + ((j + 1) & (azimuthalCount - 1));
    indices[idx + 1] = d - azimuthalCount + j;
    indices[idx + 2] = d + 1;
  }

  if (!atlrInitMesh(&sphereMesh, sizeof(vertices), vertices, sizeof(indices) / sizeof(AtlrU16), indices, &device, &singleRecordCommandContext))
  {
    ATLR_ERROR_MSG("atlrInitMesh returned 0.");
    return 0;
  }

  // index buffer
  const AtlrU16 edgeIndices[] = {0, 1, 2, 2, 1, 3};
  const AtlrU64 edgeIndicesSize = 6 * sizeof(AtlrU16);
  const VkBufferUsageFlags indexUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  const VkMemoryPropertyFlags memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (!atlrInitBuffer(&edgeDetectIndexBuffer, edgeIndicesSize, indexUsage, memoryProperty, &device) ||
      !atlrStageBuffer(&edgeDetectIndexBuffer, 0, edgeIndicesSize, edgeIndices, &singleRecordCommandContext))
  {
    ATLR_ERROR_MSG("Failed to init and stage index buffer.");
    return 0;
  }

  if(!atlrInitPerspectiveCameraHostGLFW(&camera, MAX_FRAMES_IN_FLIGHT, 45, 0.1f, 100.0f, &device))
  {
    ATLR_ERROR_MSG("atlrInitPerspectiveCameraHostGLFW returned 0.");
    return 0;
  }
  {
    const AtlrVec3 eyePos = {{8.0f, 0.0f, 0.0f}};
    const AtlrVec3 targetPos = {{0.0f, 0.0f, 0.0f}};
    const AtlrVec3 worldUpDir = {{0.0f, 0.0f, 1.0f}};
    atlrPerspectiveCameraLookAtHostGLFW(&camera, &eyePos, &targetPos, &worldUpDir);
  }

  if (!initDescriptor())
  {
    ATLR_ERROR_MSG("initDescriptor returned 0.");
    return 0;
  }

  if (!initPipelines())
  {
    ATLR_ERROR_MSG("initPipelines returned 0.");
    return 0;
  }

  return 1;
}

static void deinitGooch()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Gooch' demo ...");

  vkDeviceWaitIdle(device.logical);

  deinitPipelines();
  deinitDescriptor();
  atlrDeinitPerspectiveCameraHostGLFW(&camera);
  atlrDeinitBuffer(&edgeDetectIndexBuffer);
  atlrDeinitMesh(&sphereMesh);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSingleRecordCommandContext(&singleRecordCommandContext);
  atlrDeinitOffscreenCanvas(&offscreenCanvas, 1);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
}

int main()
{
  if (!initGooch())
  {
    ATLR_FATAL_MSG("initGooch returned 0.");
    return -1;
  }

  // parameters for the cube's rotation
  
  GLFWwindow* window = instance.data;
  while(!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    // begin recording
    if (!atlrBeginFrameCommandsHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrBeginFrameCommands returned 0.");
      return -1;
    }
    const VkCommandBuffer commandBuffer = atlrGetFrameCommandContextCommandBufferHostGLFW(&commandContext); 
    
     // start offscreen render pass
    atlrOffscreenCanvasBeginRenderPass(&offscreenCanvas, commandBuffer);

    // update camera
    atlrUpdatePerspectiveCameraHostGLFW(&camera, commandContext.currentFrame);
    // bind camera descriptor set
    vkCmdBindDescriptorSets(commandBuffer, goochPipeline.bindPoint, goochPipeline.layout, 0, 1, camera.descriptorSets + commandContext.currentFrame, 0, NULL);

    // draw sphere with gooch lighting
    vkCmdBindPipeline(commandBuffer, goochPipeline.bindPoint, goochPipeline.pipeline);
    atlrBindMesh(&sphereMesh, commandBuffer);
    atlrDrawMesh(&sphereMesh, commandBuffer);

     // end offscreen render pass
    atlrOffscreenCanvasEndRenderPass(commandBuffer);

    // begin frame render pass
    if (!atlrFrameCommandContextBeginRenderPassHostGLFW(&commandContext))
    {
      ATLR_FATAL_MSG("atlrFrameCommandContextBeginRenderPassHostGLFW returned 0.");
      return -1;
    }

    // give the sphere some edges
    vkCmdBindDescriptorSets(commandBuffer, edgeDetectPipeline.bindPoint, edgeDetectPipeline.layout, 0, 1, descriptorSets + commandContext.currentFrame, 0, NULL);
    vkCmdBindPipeline(commandBuffer, edgeDetectPipeline.bindPoint, edgeDetectPipeline.pipeline);
    vkCmdBindIndexBuffer(commandBuffer, edgeDetectIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

    // end frame render pass
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

  deinitGooch();

  return 0;
}
