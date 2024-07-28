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

AtlrU8 atlrInitBuffer(AtlrBuffer* restrict buffer, const AtlrU64 size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties,
		      const AtlrDevice* device)
{
  const VkBufferCreateInfo bufferInfo =
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };
  if (vkCreateBuffer(device->logical, &bufferInfo, device->instance->allocator, &buffer->buffer) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateBuffer did not return VK_SUCCESS.");
    return 0;
  }

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(device->logical, buffer->buffer, &memoryRequirements);
  AtlrU32 memoryTypeIndex;
  if (!atlrGetVulkanMemoryTypeIndex(&memoryTypeIndex, device->physical, memoryRequirements.memoryTypeBits, properties))
  {
    ATLR_ERROR_MSG("atlrGetVulkanMemoryTypeIndex returned 0.");
    return 0;
  }
  const VkMemoryAllocateInfo memoryAllocateInfo =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = NULL,
    .allocationSize = memoryRequirements.size,
    .memoryTypeIndex = memoryTypeIndex
  };
  if (vkAllocateMemory(device->logical, &memoryAllocateInfo, device->instance->allocator, &buffer->memory) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkAllocateMemory did not return VK_SUCCESS.");
    return 0;
  }

  if (vkBindBufferMemory(device->logical, buffer->buffer, buffer->memory, 0) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkBindBufferMemory did not return VK_SUCCESS.");
    return 0;
  }

  buffer->data = NULL;

  return 1;
}

void atlrDeinitBuffer(AtlrBuffer* restrict buffer, const AtlrDevice* device)
{
  vkFreeMemory(device->logical, buffer->memory, device->instance->allocator);
  vkDestroyBuffer(device->logical, buffer->buffer, device->instance->allocator);
}

AtlrU8 atlrMapBuffer(AtlrBuffer* restrict buffer, const AtlrU64 offset, const AtlrU64 size, const VkMemoryMapFlags flags,
		     const AtlrDevice* device)
{
  if (vkMapMemory(device->logical, buffer->memory, offset, size, flags, &buffer->data) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkMapMemory did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

void atlrUnmapBuffer(const AtlrBuffer* restrict buffer,
		     const AtlrDevice* device)
{
  vkUnmapMemory(device->logical, buffer->memory);
}

AtlrU8 atlrLoadBuffer(AtlrBuffer* restrict buffer, const AtlrU64 offset, const AtlrU64 size, const VkMemoryMapFlags flags, const void* restrict data,
		      const AtlrDevice* restrict device)
{
  if (!atlrMapBuffer(buffer, offset, size, flags, device))
  {
    ATLR_ERROR_MSG("atlrMapBuffer returned 0.");
    return 0;
  }
  memcpy(buffer->data, data, size);
  atlrUnmapBuffer(buffer, device);

  return 1;
}

AtlrU8 atlrCopyBuffer(const AtlrBuffer* restrict dst, const AtlrBuffer* restrict src, const AtlrU64 dstOffset, const AtlrU64 srcOffset, const AtlrU64 size,
		      const AtlrDevice* restrict device, const AtlrSingleRecordCommandContext* commandContext)
{
  VkCommandBuffer commandBuffer;
  if (!atlrBeginSingleRecordCommands(&commandBuffer, commandContext))
  {
    ATLR_ERROR_MSG("atlrBeginSingleRecordCommands returned 0.");
    return 0;
  }

  const VkBufferCopy copyRegion =
  {
    .srcOffset = srcOffset,
    .dstOffset = dstOffset,
    .size = size
  };
  vkCmdCopyBuffer(commandBuffer, src->buffer, dst->buffer, 1, &copyRegion);

  if (!atlrEndSingleRecordCommands(commandBuffer, commandContext))
  {
    ATLR_ERROR_MSG("atlrEndSingleRecordCommands returned 0.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrCopyBufferToImage(const AtlrBuffer* buffer, const AtlrImage* restrict image,
			     const VkOffset2D* offset, const VkExtent2D* extent,
			     const AtlrSingleRecordCommandContext* commandContext, const AtlrDevice* device)
{
  VkCommandBuffer commandBuffer;
  if (!atlrBeginSingleRecordCommands(&commandBuffer, commandContext))
  {
    ATLR_ERROR_MSG("atlrBeginSingleRecordCommands returned 0.");
    return 0;
  }

  const VkBufferImageCopy copyRegion =
  {
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    .imageSubresource = (VkImageSubresourceLayers)
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .baseArrayLayer = 0,
      .layerCount = image->layerCount
    },
    .imageOffset = (VkOffset3D)
    {
      .x = offset->x,
      .y = offset->y,
      .z = 0
    },
    .imageExtent = (VkExtent3D)
    {
      .width = extent->width,
      .height = extent->height,
      .depth = 1
    }
  };
  vkCmdCopyBufferToImage(commandBuffer, buffer->buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  if (!atlrEndSingleRecordCommands(commandBuffer, commandContext))
  {
    ATLR_ERROR_MSG("atlrEndSingleRecordCommands returned 0.");
    return 0;
  }

  return 1;
}
