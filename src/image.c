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

static VkFormat getSupportedImageFormat(const VkPhysicalDevice physical,
				   AtlrU32 formatChoiceCount, const VkFormat* restrict formatChoices, VkImageTiling tiling, VkFormatFeatureFlags features)
{
  for (AtlrU32 i = 0; i < formatChoiceCount; i++)
  {
    const VkFormat format = formatChoices[i];
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(physical, format, &properties);
    if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
      return format;
    else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
      return format;
  }

  return VK_FORMAT_UNDEFINED;
}

AtlrU8 atlrInitImageView(VkImageView* restrict imageView, const VkImage image,
			 const VkImageViewType viewType, const VkFormat format, const VkImageAspectFlags aspectFlags, const AtlrU32 layerCount,
			 const AtlrDevice* restrict device)
{
  const VkImageViewCreateInfo imageViewInfo =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .image = image,
    .viewType = viewType,
    .format = format,
    .components = (VkComponentMapping)
    {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY
    },
    .subresourceRange = (VkImageSubresourceRange)
    {
      .aspectMask = aspectFlags,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = layerCount
    }
  };
  if(vkCreateImageView(device->logical, &imageViewInfo, device->instance->allocator, imageView) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("vkCreateImageView did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

void atlrDeinitImageView(const VkImageView imageView,
			 const AtlrDevice* restrict device)
{
  vkDestroyImageView(device->logical, imageView, device->instance->allocator);
}

AtlrU8 atlrInitImage(AtlrImage* restrict image, const AtlrU32 width, const AtlrU32 height,
		     const AtlrU32 layerCount, const VkFormat format, const VkImageTiling tiling, const VkImageUsageFlags usage,
		     const VkMemoryPropertyFlags properties, const VkImageViewType viewType, const VkImageAspectFlags aspectFlags,
		     const AtlrDevice* restrict device)
{
  const VkImageCreateInfo imageInfo =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = format,
    .extent = (VkExtent3D)
    {
      .width = width,
      .height = height,
      .depth = 1
    },
    .mipLevels = 1,
    .arrayLayers = layerCount,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = tiling,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };
  if (vkCreateImage(device->logical, &imageInfo, device->instance->allocator, &image->image) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("vkCreateImage did not return VK_SUCCESS.");
    return 0;
  }
  
  image->width = width;
  image->height = height;
  image->layerCount = layerCount;

  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements(device->logical, image->image, &memoryRequirements);
  AtlrU32 memoryTypeIndex;
  if (!atlrGetVulkanMemoryTypeIndex(&memoryTypeIndex, device->physical, memoryRequirements.memoryTypeBits, properties))
  {
    ATLR_LOG_ERROR("atlrGetVulkanMemoryTypeIndex returned 0.");
    return 0;
  }
  const VkMemoryAllocateInfo memoryAllocateInfo =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = NULL,
    .allocationSize = memoryRequirements.size,
    .memoryTypeIndex = memoryTypeIndex
  };
  if (vkAllocateMemory(device->logical, &memoryAllocateInfo, device->instance->allocator, &image->memory) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("vkAllocateMemory did not return VK_SUCCESS.");
    return 0;
  }

  if (vkBindImageMemory(device->logical, image->image, image->memory, 0) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("vkBindImageMemory did not return VK_SUCCESS.");
    return 0;
  }

  if (!atlrInitImageView(&image->imageView, image->image, viewType, format, aspectFlags, layerCount, device))
  {
    ATLR_LOG_ERROR("atlrInitImageView returned 0.");
    return 0;
  }

  return 1;
}

void atlrDeinitImage(const AtlrImage* restrict image, const AtlrDevice* restrict device)
{
  atlrDeinitImageView(image->imageView, device);
  vkFreeMemory(device->logical, image->memory, device->instance->allocator);
  vkDestroyImage(device->logical, image->image, device->instance->allocator);
}

AtlrU8 atlrInitDepthImage(AtlrImage* restrict image, const AtlrU32 width, const AtlrU32 height,
			  const AtlrDevice* restrict device)
{
  const VkFormat formatChoices[3] =
  {
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT
  };
  const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  const VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  const VkFormat format = getSupportedImageFormat(device->physical, 3, formatChoices, tiling, features);
  if (format == VK_FORMAT_UNDEFINED)
  {
    ATLR_LOG_ERROR("getSupportedImageFormat returned VK_FORMAT_UNDEFINED.");
    return 0;
  }

  if (!atlrInitImage(image, width, height, 1, format, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, device))
  {
    ATLR_LOG_ERROR("atlrInitImage returned 0.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrTransitionImageLayout(const AtlrImage* restrict image, const VkImageLayout oldLayout, const VkImageLayout newLayout,
				 const AtlrSingleRecordCommandContext* restrict commandContext, const AtlrDevice* restrict device)
{
  VkCommandBuffer commandBuffer;
  if (!atlrBeginSingleRecordCommands(&commandBuffer, commandContext, device))
  {
    ATLR_LOG_ERROR("atlrBeginSingleRecordCommands returned 0.");
    return 0;
  }
  
  VkImageMemoryBarrier barrier =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .pNext = NULL,
    .oldLayout = oldLayout,
    .newLayout = newLayout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image->image,
    .subresourceRange = (VkImageSubresourceRange)
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = image->layerCount
    }
  };
  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    ATLR_LOG_ERROR("Invalid image layout transition.");
    return 0;
  }

  vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &barrier);

  if (!atlrEndSingleRecordCommands(commandBuffer, commandContext, device))
  {
    ATLR_LOG_ERROR("atlrEndSingleRecordCommands returned 0.");
    return 0;
  }

  return 1;
}
