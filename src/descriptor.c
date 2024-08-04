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

#include "antler.h"

VkDescriptorSetLayoutBinding atlrInitDescriptorSetLayoutBinding(const AtlrU32 binding, const VkDescriptorType type, const VkShaderStageFlags stageFlags)
{
  return (VkDescriptorSetLayoutBinding)
  {
    .binding = binding,
    .descriptorType = type,
    .descriptorCount = 1,
    .stageFlags = stageFlags,
    .pImmutableSamplers = NULL
  };
}

AtlrU8 atlrInitDescriptorSetLayout(AtlrDescriptorSetLayout* restrict setLayout, const AtlrU32 bindingCount, const VkDescriptorSetLayoutBinding* restrict bindings,
				   const AtlrDevice* restrict device)
{
  setLayout->device = device;
  
  const VkDescriptorSetLayoutCreateInfo setLayoutInfo =
  {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .bindingCount = bindingCount,
    .pBindings = bindings
  };

  if(vkCreateDescriptorSetLayout(device->logical, &setLayoutInfo, device->instance->allocator, &setLayout->layout) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateDescriptorSetLayout did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

void atlrDeinitDescriptorSetLayout(const AtlrDescriptorSetLayout* restrict setLayout)
{
  const AtlrDevice* device = setLayout->device;
  vkDestroyDescriptorSetLayout(device->logical, setLayout->layout, device->instance->allocator);
}

VkDescriptorPoolSize atlrInitDescriptorPoolSize(const VkDescriptorType type, const AtlrU32 descriptorCount)
{
  return (VkDescriptorPoolSize)
  {
    .type = type,
    .descriptorCount = descriptorCount
  };
}

AtlrU8 atlrInitDescriptorPool(AtlrDescriptorPool* restrict pool, const AtlrU32 maxSets, const AtlrU32 poolSizeCount, const VkDescriptorPoolSize* restrict poolSizes,
			      const AtlrDevice* restrict device)
{
  pool->device = device;
  
  const VkDescriptorPoolCreateInfo poolInfo =
  {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .maxSets = maxSets,
    .poolSizeCount = poolSizeCount,
    .pPoolSizes = poolSizes
  };
  
  if (vkCreateDescriptorPool(device->logical, &poolInfo, device->instance->allocator, &pool->pool) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateDescriptorPool did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

void atlrDeinitDescriptorPool(const AtlrDescriptorPool* restrict pool)
{
  const AtlrDevice* device = pool->device;
  vkDestroyDescriptorPool(device->logical, pool->pool, device->instance->allocator);
}

AtlrU8 atlrAllocDescriptorSets(const AtlrDescriptorPool* restrict pool, const AtlrU32 setCount, const VkDescriptorSetLayout* restrict setLayouts, VkDescriptorSet* restrict sets)
{
  VkDescriptorSetAllocateInfo setInfo =
  {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = NULL,
    .descriptorPool = pool->pool,
    .descriptorSetCount = setCount,
    .pSetLayouts = setLayouts
  };
  
  if (vkAllocateDescriptorSets(pool->device->logical, &setInfo, sets) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkAllocateDescriptorSets did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

VkDescriptorBufferInfo atlrInitDescriptorBufferInfo(const AtlrBuffer* restrict buffer, const AtlrU64 size)
{
  return (VkDescriptorBufferInfo)
  {
    .buffer = buffer->buffer,
    .offset = 0,
    .range = size
  };
}

VkDescriptorImageInfo atlrInitDescriptorImageInfo(const AtlrImage* restrict image, const VkSampler sampler, const VkImageLayout imageLayout)
{
  return (VkDescriptorImageInfo)
  {
    .sampler = sampler,
    .imageView = image->imageView,
    .imageLayout = imageLayout
  };
}

VkWriteDescriptorSet atlrWriteBufferDescriptorSet(const VkDescriptorSet set, const AtlrU32 binding, const VkDescriptorType type, const VkDescriptorBufferInfo* restrict bufferInfo)
{
  return (VkWriteDescriptorSet)
  {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = NULL,
    .dstSet = set,
    .dstBinding = binding,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = type,
    .pImageInfo = NULL,
    .pBufferInfo = bufferInfo,
    .pTexelBufferView = NULL
  };
}

VkWriteDescriptorSet atlrWriteImageDescriptorSet(const VkDescriptorSet set, const AtlrU32 binding, const VkDescriptorType type, const VkDescriptorImageInfo* restrict imageInfo)
{
  return (VkWriteDescriptorSet)
  {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = NULL,
    .dstSet = set,
    .dstBinding = binding,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = type,
    .pImageInfo = imageInfo,
    .pBufferInfo = NULL,
    .pTexelBufferView = NULL
  };
}
