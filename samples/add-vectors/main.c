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

typedef struct Pipeline
{
  VkPipelineLayout layout;
  VkPipeline pipeline;
  
} Pipeline;

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSingleRecordCommandContext commandContext;
static AtlrBuffer storageBuffers[3];
static VkDescriptorSetLayout descriptorSetLayout;
static VkDescriptorPool descriptorPool;
static VkDescriptorSet descriptorSet;
static Pipeline pipeline;

#define VECTOR_DIM 7

static AtlrU8 initStorageBuffers()
{
  const AtlrU64 size = sizeof(float) * VECTOR_DIM;
  VkBufferUsageFlags storageUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  const VkMemoryPropertyFlags storageMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  
  for (AtlrU8 i = 0; i < 3; i++)
  {
    AtlrBuffer* storageBuffer = storageBuffers + i;
    if (i < 2)
      storageUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    else
      storageUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    if (!atlrInitBuffer(storageBuffer, size, storageUsage, storageMemoryProperties, &device))
    {
      ATLR_ERROR_MSG("atlrInitBuffer returned 0.");
      return 0;
    }
  }

  return 1;
}

static void deinitStorageBuffers()
{
  for (AtlrU8 i = 0; i < 3; i++)
    atlrDeinitBuffer(storageBuffers + i, &device);
}

static AtlrU8 initDescriptor()
{
  VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[3];
  for (AtlrU8 i = 0; i < 3; i++)
    descriptorSetLayoutBindings[i] = (VkDescriptorSetLayoutBinding)
    {
      .binding = i,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = NULL
    };
  const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
  {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .bindingCount = 3,
    .pBindings = descriptorSetLayoutBindings
  };
  if(vkCreateDescriptorSetLayout(device.logical, &descriptorSetLayoutInfo, instance.allocator, &descriptorSetLayout) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateDescriptorSetLayout did not return VK_SUCCESS.");
    return 0;
  }

  const VkDescriptorPoolSize poolSize =
  {
    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 3
  };
  const VkDescriptorPoolCreateInfo descriptorPoolInfo =
  {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .maxSets = 1,
    .poolSizeCount = 1,
    .pPoolSizes = &poolSize
  };
  if (vkCreateDescriptorPool(device.logical, &descriptorPoolInfo, instance.allocator, &descriptorPool) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateDescriptorPool did not return VK_SUCCESS.");
    return 0;
  }

  const VkDescriptorSetAllocateInfo descriptorSetAllocInfo =
  {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = NULL,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = 1,
    .pSetLayouts = &descriptorSetLayout
  };
  if (vkAllocateDescriptorSets(device.logical, &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkAllocateDescriptorSets did not return VK_SUCCESS.");
    return 0;
  }

  VkDescriptorBufferInfo bufferInfos[3];
  VkWriteDescriptorSet descriptorWrites[3];
  for (AtlrU8 i = 0; i < 3; i++)
  {
    const VkDescriptorBufferInfo bufferInfo =
    {
      .buffer = storageBuffers[i].buffer,
      .offset = 0,
      .range = sizeof(float) * VECTOR_DIM
    };
    memcpy(bufferInfos + i, &bufferInfo, sizeof(VkDescriptorBufferInfo));
    const VkWriteDescriptorSet descriptorWrite =
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = NULL,
      .dstSet = descriptorSet,
      .dstBinding = i,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo = NULL,
      .pBufferInfo = bufferInfos + i,
      .pTexelBufferView = NULL
    };
    memcpy(descriptorWrites + i, &descriptorWrite, sizeof(VkWriteDescriptorSet));
  }
  vkUpdateDescriptorSets(device.logical, 3, descriptorWrites, 0, NULL);

  return 1;
}

static void deinitDescriptor()
{
  vkDestroyDescriptorPool(device.logical, descriptorPool, instance.allocator);
  descriptorSet = VK_NULL_HANDLE;
  vkDestroyDescriptorSetLayout(device.logical, descriptorSetLayout, instance.allocator);
}

static AtlrU8 initPipeline()
{
  VkShaderModule module = atlrInitShaderModule("add-comp.spv", &device);
  VkPipelineShaderStageCreateInfo stageInfo =  atlrInitPipelineComputeShaderStageInfo(module);

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    atlrInitPipelineLayoutInfo(1, &descriptorSetLayout);
  if (vkCreatePipelineLayout(device.logical, &pipelineLayoutInfo, instance.allocator, &pipeline.layout) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreatePipelineLayout did not return VK_SUCCESS.");
    return 0;
  }

  const VkComputePipelineCreateInfo pipelineInfo =
  {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = stageInfo,
    .layout = pipeline.layout,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };
  if (vkCreateComputePipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineInfo, instance.allocator, &pipeline.pipeline) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateComputePipelines returned 0.");
    return 0;
  }
  
  atlrDeinitShaderModule(module, &device);

  return 1;
}

static void deinitPipeline()
{
  vkDestroyPipelineLayout(device.logical, pipeline.layout, instance.allocator);
  vkDestroyPipeline(device.logical, pipeline.pipeline, instance.allocator);
}
  
static AtlrU8 initAddVectors()
{
  atlrLog(ATLR_LOG_INFO, "Starting 'Adding Vectors' demo ...");

  if (!atlrInitInstanceHostHeadless(&instance, "Adding Vectors Demo"))
  {
    ATLR_ERROR_MSG("atlrInitHostGLFW returned 0.");
    return 0;
  }

  AtlrDeviceCriteria deviceCriteria;
  atlrInitDeviceCriteria(deviceCriteria);
  atlrSetDeviceCriterion(deviceCriteria,
			 ATLR_DEVICE_CRITERION_QUEUE_FAMILY_COMPUTE_SUPPORT,
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

  const AtlrU32 computeQueueFamilyIndex = device.queueFamilyIndices.graphicsComputeIndex;
  if (!atlrInitSingleRecordCommandContext(&commandContext, computeQueueFamilyIndex, &device))
  {
    ATLR_ERROR_MSG("atlrInitSingleRecordCommandContext returned 0.");
    return 0;
  }

  if (!initStorageBuffers())
  {
    ATLR_ERROR_MSG("initStorageBuffers returned 0.");
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

  return 1;
}

static void deinitAddVectors()
{
  atlrLog(ATLR_LOG_INFO, "Ending 'Adding Vectors' demo ...");

  vkDeviceWaitIdle(device.logical);
  
  deinitPipeline();
  deinitDescriptor();
  deinitStorageBuffers();
  atlrDeinitSingleRecordCommandContext(&commandContext);
  atlrDeinitDeviceHost(&device);
  atlrDeinitInstanceHostHeadless(&instance);
}

int main()
{
  if (!initAddVectors())
  {
    ATLR_FATAL_MSG("initAddVectors returned 0.");
    return -1;
  }

  float result[VECTOR_DIM];
  float inputVecs[2][VECTOR_DIM];
  const AtlrU64 size = sizeof(float) * VECTOR_DIM;

  unsigned int seed;
  char choice;

  do
  {
    printf("Enter a seed value: ");
    scanf("%u", &seed);

    atlrLog(ATLR_LOG_INFO, "Adding two random-value vectors of dimension %d: C = A + B ...",  VECTOR_DIM);
    
    srand(seed);
    for (AtlrU8 i = 0; i < VECTOR_DIM; i++)
    {
      inputVecs[0][i] = (float)rand() / (float)RAND_MAX;
      inputVecs[1][i] = (float)rand() / (float)RAND_MAX;
    }

    for (AtlrU8 i = 0; i < 2; i++)
    {
      if (!atlrStageBuffer(storageBuffers + i, 0, size, inputVecs[i], &device, &commandContext))
      {
	ATLR_ERROR_MSG("atlrLoadBuffer returned 0.");
	return 0;
      }
    }
  
    VkCommandBuffer commandBuffer;
    if (!atlrBeginSingleRecordCommands(&commandBuffer, &commandContext))
    {
      ATLR_FATAL_MSG("atlrBeginSingleRecordCommands returned 0.");
      return -1;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0, 1, &descriptorSet, 0, NULL);
    vkCmdDispatch(commandBuffer, VECTOR_DIM, 1, 1);
  
    if (!atlrEndSingleRecordCommands(commandBuffer, &commandContext))
    {
      ATLR_FATAL_MSG("atlrEndSingleRecordCommands returned 0.");
      return -1;
    }

    if (!atlrReadbackBuffer(storageBuffers + 2, 0, sizeof(float) * VECTOR_DIM, result, &device, &commandContext))
    {
      ATLR_FATAL_MSG("atlrReadbackBuffer returned 0.");
      return -1;
    }
    
    for (AtlrU8 i = 0; i < VECTOR_DIM; i++)
      atlrLog(ATLR_LOG_INFO, "A[%d] = %f; B{%d] = %f, C[%d] = %f", i, inputVecs[0][i], i, inputVecs[1][i], i, result[i]);

    printf("Do you want to enter a new seed? (y/n): ");
    scanf(" %c", &choice);

  } while (choice == 'y' || choice == 'Y');

  deinitAddVectors();

  return 0;
}
