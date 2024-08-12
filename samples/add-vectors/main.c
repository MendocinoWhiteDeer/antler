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

static AtlrInstance instance;
static AtlrDevice device;
static AtlrSingleRecordCommandContext commandContext;
static AtlrBuffer storageBuffers[3];
static AtlrDescriptorSetLayout descriptorSetLayout;
static AtlrDescriptorPool descriptorPool;
static VkDescriptorSet descriptorSet;
static AtlrPipeline pipeline;

#define VECTOR_DIM 7

static AtlrU8 initStorageBuffers()
{
  const AtlrU64 size = sizeof(float) * VECTOR_DIM;
  VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  
  for (AtlrU8 i = 0; i < 3; i++)
  {
    AtlrBuffer* storageBuffer = storageBuffers + i;
    if (i < 2)
      usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    else
      usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    if (!atlrInitBuffer(storageBuffer, size, usage, memoryProperties, &device))
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
    atlrDeinitBuffer(storageBuffers + i);
}

static AtlrU8 initDescriptor()
{
  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  
  VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[3];
  for (AtlrU8 i = 0; i < 3; i++)
    descriptorSetLayoutBindings[i] = atlrInitDescriptorSetLayoutBinding(i, type, VK_SHADER_STAGE_COMPUTE_BIT);
  if (!atlrInitDescriptorSetLayout(&descriptorSetLayout, 3, descriptorSetLayoutBindings, &device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorSetLayout returned 0.");
    return 0;
  }

  const VkDescriptorPoolSize poolSize = atlrInitDescriptorPoolSize(type, 3);
  if (!atlrInitDescriptorPool(&descriptorPool, 1, 1, &poolSize, &device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorPool returned 0.");
    return 0;
  }

  if (!atlrAllocDescriptorSets(&descriptorPool, 1, &descriptorSetLayout.layout, &descriptorSet))
  {
    ATLR_ERROR_MSG("atlrAllocDescriptorSets returned 0.");
    return 0;
  }

  VkDescriptorBufferInfo bufferInfos[3];
  VkWriteDescriptorSet descriptorWrites[3];
  for (AtlrU8 i = 0; i < 3; i++)
  {
    bufferInfos[i] = atlrInitDescriptorBufferInfo(storageBuffers + i, sizeof(float) * VECTOR_DIM);
    descriptorWrites[i] = atlrWriteBufferDescriptorSet(descriptorSet, i, type, bufferInfos + i);
  }
  vkUpdateDescriptorSets(device.logical, 3, descriptorWrites, 0, NULL);

  return 1;
}

static void deinitDescriptor()
{
  atlrDeinitDescriptorPool(&descriptorPool);
  descriptorSet = VK_NULL_HANDLE;
  atlrDeinitDescriptorSetLayout(&descriptorSetLayout);
}

static AtlrU8 initPipeline()
{
  VkShaderModule module = atlrInitShaderModule("add-comp.spv", &device);
  VkPipelineShaderStageCreateInfo stageInfo =  atlrInitPipelineComputeShaderStageInfo(module);

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(1, &descriptorSetLayout.layout, 0, NULL);

  if (!atlrInitComputePipeline(&pipeline, &stageInfo, &pipelineLayoutInfo, &device))
  {
    ATLR_ERROR_MSG("atlrInitComputePipeline returned 0.");
    return 0;
  }
  
  atlrDeinitShaderModule(module, &device);

  return 1;
}

static void deinitPipeline()
{
  atlrDeinitPipeline(&pipeline);
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
      if (!atlrStageBuffer(storageBuffers + i, 0, size, inputVecs[i], &commandContext))
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

    if (!atlrReadbackBuffer(storageBuffers + 2, 0, sizeof(float) * VECTOR_DIM, result, &commandContext))
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
