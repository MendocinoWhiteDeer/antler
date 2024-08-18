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

#define MAX_FRAMES_IN_FLIGHT 2

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSwapchain swapchain;
static AtlrFrameCommandContext commandContext;
static struct
{
  float time;            float padding;
  float resolution[2];
  
} uniformData;
static AtlrBuffer* uniformBuffers;
static AtlrDescriptorSetLayout descriptorSetLayout;
static AtlrDescriptorPool descriptorPool;
static VkDescriptorSet* descriptorSets;
static AtlrBuffer indexBuffer;
static AtlrPipeline pipeline;

static const char* vertexShaderSource =
  "#version 460\n"
  "const vec2 positions[4] = vec2[4](vec2(-1.0f,-1.0f), vec2(1.0f,-1.0f), vec2(-1.0f,1.0f), vec2(1.0f,1.0f));\n"
  "void main() { gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f); }";

static const char* fragmentShaderSourceHeader =
  "#version 460\n"
  "layout(location = 0) out vec4 outColor;\n"
  "layout(binding = 0, set = 0) uniform AtlrUniform { float time; vec2 resolution; } atlr;\n";

static const char* fragmentShaderSourceEntryPoint =
  "\nvoid main() { atlrFragment(outColor, gl_FragCoord.xy); }";

static AtlrU8 initDescriptor()
{
  const AtlrU64 size = sizeof(uniformData);

  uniformBuffers = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(AtlrBuffer));
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

  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  const VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = atlrInitDescriptorSetLayoutBinding(0, type, VK_SHADER_STAGE_FRAGMENT_BIT);
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

  descriptorSets = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDescriptorSet));
  VkDescriptorSetLayout* setLayouts = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDescriptorSetLayout));
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) setLayouts[i] = descriptorSetLayout.layout;
  if (!atlrAllocDescriptorSets(&descriptorPool, MAX_FRAMES_IN_FLIGHT, setLayouts, descriptorSets))
  {
    ATLR_ERROR_MSG("atlrAllocDescriptorSets returned 0.");
    free(setLayouts);
    return 0;
  }
  free(setLayouts);

  VkDescriptorBufferInfo* bufferInfos = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDescriptorBufferInfo));
  VkWriteDescriptorSet* descriptorWrites = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkWriteDescriptorSet));
  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    bufferInfos[i] = atlrInitDescriptorBufferInfo(uniformBuffers + i, sizeof(uniformData));
    descriptorWrites[i] = atlrWriteBufferDescriptorSet(descriptorSets[i], 0, type, bufferInfos + i);
  }
  vkUpdateDescriptorSets(device.logical, MAX_FRAMES_IN_FLIGHT, descriptorWrites, 0, NULL);
  free(bufferInfos);
  free(descriptorWrites);

  return 1;
}

static void deinitDescriptor()
{
  atlrDeinitDescriptorPool(&descriptorPool);
  free(descriptorSets);
  atlrDeinitDescriptorSetLayout(&descriptorSetLayout);

  for (AtlrU8 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    atlrDeinitBuffer(uniformBuffers + i);
  free(uniformBuffers);
}

static AtlrU8 initPipeline(const char* fragmentShaderPath)
{
  // vertex shader module
  atlrLog(ATLR_LOG_DEBUG, "Creating vertex shader module ...");
  VkShaderModule vertexModule;
  {
    AtlrSpirVBinary bin = {};
    if (!atlrInitSpirVBinary(&bin, GLSLANG_STAGE_VERTEX, vertexShaderSource, "vertex"))
    {
      ATLR_ERROR_MSG("atlrCompileShader returned 0.");
      return 0;
    }
    
    const VkShaderModuleCreateInfo moduleInfo =
    {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = bin.codeSize,
      .pCode = bin.code
    };
    if (vkCreateShaderModule(device.logical, &moduleInfo, instance.allocator, &vertexModule) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkCreateShaderModule did not return VK_SUCCESS.");
      return 0;
    }

    atlrDeinitSpirVBinary(&bin);
  }

  // fragment shader module
  VkShaderModule fragmentModule;
  atlrLog(ATLR_LOG_DEBUG, "Creating fragment shader module ...");
  {
    FILE* file = fopen(fragmentShaderPath, "r");
    if (!file)
    {
      ATLR_ERROR_MSG("Failed to open file at path \"%s\".", fragmentShaderPath);
      return 0;
    }
    fseek(file, 0, SEEK_END);
    const size_t bodySize = ftell(file);
    rewind(file);
    char* body = malloc((bodySize + 1) * sizeof(char));
    fread(body, bodySize, 1, file);
    body[bodySize] = '\0';
    fclose(file);

    const long int glslSize = (bodySize + 1 + strlen(fragmentShaderSourceHeader) + strlen(fragmentShaderSourceEntryPoint)) * sizeof(char);
    char* glsl = malloc(glslSize);
    strcpy(glsl, fragmentShaderSourceHeader);
    strcat(glsl, body);
    free(body);
    strcat(glsl, fragmentShaderSourceEntryPoint);

    AtlrSpirVBinary bin = {};
    if (!atlrInitSpirVBinary(&bin, GLSLANG_STAGE_FRAGMENT, glsl, fragmentShaderPath))
    {
      ATLR_ERROR_MSG("atlrCompileShader returned 0.");
      return 0;
    }
    
    const VkShaderModuleCreateInfo moduleInfo =
    {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = bin.codeSize,
      .pCode = bin.code
    };
    if (vkCreateShaderModule(device.logical, &moduleInfo, instance.allocator, &fragmentModule) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkCreateShaderModule did not return VK_SUCCESS.");
      return 0;
    }

    free(glsl);
    atlrDeinitSpirVBinary(&bin);
  }
  
  VkShaderModule modules[2] = { vertexModule, fragmentModule };
  VkPipelineShaderStageCreateInfo stageInfos[2] =
  {
    atlrInitPipelineVertexShaderStageInfo(modules[0]),
    atlrInitPipelineFragmentShaderStageInfo(modules[1])
  };

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo     = atlrInitVertexInputStateInfo(0, NULL, 0, NULL);
  const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = atlrInitPipelineInputAssemblyStateInfo();
  const VkPipelineViewportStateCreateInfo viewportInfo           = atlrInitPipelineViewportStateInfo();
  const VkPipelineRasterizationStateCreateInfo rasterizationInfo = atlrInitPipelineRasterizationStateInfo();
  const VkPipelineMultisampleStateCreateInfo multisampleInfo     = atlrInitPipelineMultisampleStateInfo(device.msaaSamples);
  const VkPipelineDepthStencilStateCreateInfo depthStencilInfo   = atlrInitPipelineDepthStencilStateInfo();
  const VkPipelineColorBlendAttachmentState colorBlendAttachment = atlrInitPipelineColorBlendAttachmentStateAlpha();
  const VkPipelineColorBlendStateCreateInfo colorBlendInfo       = atlrInitPipelineColorBlendStateInfo(&colorBlendAttachment);
  const VkPipelineDynamicStateCreateInfo dynamicInfo             = atlrInitPipelineDynamicStateInfo();

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(1, &descriptorSetLayout.layout, 0, NULL);

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
  
static AtlrU8 initFragmentShaderClient(const char* fragmentShaderPath)
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Fragment Shader Client' demo ...");

  if (!glslang_initialize_process())
  {
    ATLR_ERROR_MSG("glslang_initialize_process returned 0.");
    return 0;
  }

  if (!atlrInitInstanceHostGLFW(&instance, 800, 400, "Fragment Shader Client"))
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

  if (!atlrInitFrameCommandContextHostGLFW(&commandContext, MAX_FRAMES_IN_FLIGHT, &swapchain))
  {
    ATLR_ERROR_MSG("atlrInitFrameCommandContext returned 0.");
    return 0;
  }

  // index buffer
  AtlrSingleRecordCommandContext singleRecordCommandContext;
  if (!atlrInitSingleRecordCommandContext(&singleRecordCommandContext, device.queueFamilyIndices.graphicsComputeIndex, &device))
  {
    ATLR_ERROR_MSG("atlrInitSingleRecordCommandContext returned 0.");
    return 0;
  }
  const AtlrU16 indices[] = {0, 1, 2, 2, 1, 3};
  const AtlrU64 indicesSize = 6 * sizeof(AtlrU16);
  const VkBufferUsageFlags indexUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  const VkMemoryPropertyFlags memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (!atlrInitBuffer(&indexBuffer, indicesSize, indexUsage, memoryProperty, &device) ||
      !atlrStageBuffer(&indexBuffer, 0, indicesSize, indices, &singleRecordCommandContext))
  {
    ATLR_ERROR_MSG("Failed to init and stage index buffer.");
    return 0;
  }
  atlrDeinitSingleRecordCommandContext(&singleRecordCommandContext);

  if(!initDescriptor())
  {
    ATLR_ERROR_MSG("initDescriptor returned 0.");
  }

  if (!initPipeline(fragmentShaderPath))
  {
    ATLR_ERROR_MSG("initPipeline returned 0.");
    return 0;
  }

  return 1;
}

static void deinitFragmentShaderClient()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Fragment Shader Client' demo ...");

  vkDeviceWaitIdle(device.logical);
  
  deinitPipeline();
  deinitDescriptor();
  atlrDeinitBuffer(&indexBuffer);
  atlrDeinitFrameCommandContextHostGLFW(&commandContext);
  atlrDeinitSwapchainHostGLFW(&swapchain, 1);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostGLFW(&instance);
  glslang_finalize_process();
}


int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    ATLR_FATAL_MSG("Usage: %s <filepath>\n", argv[0]);
    return -1;
  }
  
  if (!initFragmentShaderClient(argv[1]))
  {
    ATLR_FATAL_MSG("initFragmentShaderClient returned 0.");
    return -1;
  }

  GLFWwindow* window = instance.data;
  glfwSetTime(0.0);
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

    {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      uniformData.resolution[0] = width;
      uniformData.resolution[1] = height; 
      uniformData.time = glfwGetTime();
      memcpy(uniformBuffers[commandContext.currentFrame].data, &uniformData, sizeof(uniformData));
    }
    vkCmdBindDescriptorSets(commandBuffer, pipeline.bindPoint, pipeline.layout, 0, 1, descriptorSets + commandContext.currentFrame, 0, NULL);
    
    vkCmdBindPipeline(commandBuffer, pipeline.bindPoint, pipeline.pipeline);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

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

  deinitFragmentShaderClient();

  return 0;
}
