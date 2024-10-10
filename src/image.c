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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static const VkFormat depthFormatChoices[] =
{
  VK_FORMAT_D32_SFLOAT,
  VK_FORMAT_D32_SFLOAT_S8_UINT,
  VK_FORMAT_D24_UNORM_S8_UINT
};

static VkFormat getSupportedImageFormat(const VkPhysicalDevice physical, const AtlrU32 formatChoiceCount, const VkFormat* restrict formatChoices, const VkImageTiling tiling, const VkFormatFeatureFlags features)
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

VkFormat atlrGetSupportedDepthImageFormat(const VkPhysicalDevice physical, const VkImageTiling tiling)
{
  const VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  return getSupportedImageFormat(physical, sizeof(depthFormatChoices) / sizeof(VkFormat), depthFormatChoices, tiling, features);
}

VkImageView atlrInitImageView(const VkImage image,
			      const VkImageViewType viewType, const VkFormat format, const VkImageAspectFlags aspects, const AtlrU32 layerCount,
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
      .aspectMask = aspects,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = layerCount
    }
  };
  VkImageView imageView;
  if(vkCreateImageView(device->logical, &imageViewInfo, device->instance->allocator, &imageView) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateImageView did not return VK_SUCCESS.");
    return VK_NULL_HANDLE;
  }

  return imageView;
}

void atlrDeinitImageView(const VkImageView imageView,
			 const AtlrDevice* restrict device)
{
  vkDestroyImageView(device->logical, imageView, device->instance->allocator);
}

AtlrU8 atlrTransitionImageLayout(const AtlrImage* restrict image, const VkImageLayout oldLayout, const VkImageLayout newLayout,
				 const AtlrSingleRecordCommandContext* restrict commandContext)
{
  VkCommandBuffer commandBuffer;
  if (!atlrBeginSingleRecordCommands(&commandBuffer, commandContext))
  {
    ATLR_ERROR_MSG("atlrBeginSingleRecordCommands returned 0.");
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
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    ATLR_ERROR_MSG("Invalid image layout transition.");
    return 0;
  }

  vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &barrier);

  if (!atlrEndSingleRecordCommands(commandBuffer, commandContext))
  {
    ATLR_ERROR_MSG("atlrEndSingleRecordCommands returned 0.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrInitImage(AtlrImage* restrict image, const AtlrU32 width, const AtlrU32 height,
		     const AtlrU32 layerCount, const VkSampleCountFlagBits samples, const VkFormat format, const VkImageTiling tiling, const VkImageUsageFlags usage,
		     const VkMemoryPropertyFlags properties, const VkImageViewType viewType, const VkImageAspectFlags aspects,
		     const AtlrDevice* restrict device)
{
  image->device = device;
  
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
    .samples = samples,
    .tiling = tiling,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };
  if (vkCreateImage(device->logical, &imageInfo, device->instance->allocator, &image->image) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateImage did not return VK_SUCCESS.");
    return 0;
  }

  image->format = format;
  image->width = width;
  image->height = height;
  image->layerCount = layerCount;

  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements(device->logical, image->image, &memoryRequirements);
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
  if (vkAllocateMemory(device->logical, &memoryAllocateInfo, device->instance->allocator, &image->memory) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkAllocateMemory did not return VK_SUCCESS.");
    return 0;
  }

  if (vkBindImageMemory(device->logical, image->image, image->memory, 0) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkBindImageMemory did not return VK_SUCCESS.");
    return 0;
  }

  VkImageView imageView = atlrInitImageView(image->image, viewType, format, aspects, layerCount, device);
  if (imageView == VK_NULL_HANDLE)
  {
    ATLR_ERROR_MSG("atlrInitImageView returned VK_NULL_HANDLE.");
    return 0;
  }
  image->imageView = imageView;

  return 1;
}

AtlrU8 atlrInitImageRgbaTextureFromFile(AtlrImage* image, const char* filePath,
				    const AtlrDevice* restrict device, const AtlrSingleRecordCommandContext* restrict commandContext)
{
  int width, height, channels;
  stbi_uc* pixels = stbi_load(filePath, &width, &height, &channels, STBI_rgb_alpha);
  if (!pixels)
  {
    ATLR_ERROR_MSG("stbi_load returned NULL.");
    return 0;
  }

  const VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
  const VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (!atlrInitImage(image, width, height, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, usage, memoryProperties, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, device))
  {
    ATLR_ERROR_MSG("atlrInitImage returned 0.");
    return 0;
  }

  const AtlrU64 size = width * height * 4;
  AtlrBuffer stagingBuffer;
  if (!atlrInitStagingBuffer(&stagingBuffer, size, device))
  {
    ATLR_ERROR_MSG("atlrInitStagingBuffer returned 0.");
    return 0;
  }

  if (!atlrWriteBuffer(&stagingBuffer, 0, size, 0, pixels))
  {
    ATLR_ERROR_MSG("atlrWriteBuffer returned 0.");
    return 0;
  }
  stbi_image_free(pixels);

  const VkImageLayout initLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
  const VkImageLayout secondLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  const VkImageLayout finalLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  const VkOffset2D offset          = {.x = 0, .y = 0};
  const VkExtent2D extent          = {.width = width, .height = height};
  if (!atlrTransitionImageLayout(image, initLayout, secondLayout, commandContext) ||
      !atlrCopyBufferToImage(&stagingBuffer, image, &offset, &extent, commandContext) ||
      !atlrTransitionImageLayout(image, secondLayout, finalLayout, commandContext))
  {
    ATLR_ERROR_MSG("Failed to stage texture image.");
    return 0;
  }

  atlrDeinitBuffer(&stagingBuffer);

  return 1;
}

void atlrDeinitImage(const AtlrImage* restrict image)
{
  const AtlrDevice* device = image->device;
  atlrDeinitImageView(image->imageView, device);
  vkFreeMemory(device->logical, image->memory, device->instance->allocator);
  vkDestroyImage(device->logical, image->image, device->instance->allocator);
}

#ifdef ATLR_DEBUG
void atlrSetImageName(const AtlrImage* restrict image, const char* restrict imageName)
{
  size_t n = strlen(imageName) + 1;
  const char* imageFooter = " ; VkImage";
  const char* memoryFooter = " ; VkDeviceMemory";
  const char* imageViewFooter = " ; VkImageView";
  
  char* imageString = malloc(n + strlen(imageFooter));
  strcpy(imageString, imageName);
  strcat(imageString, imageFooter);

  char* memoryString = malloc(n + strlen(memoryFooter));
  strcpy(memoryString, imageName);
  strcat(memoryString, memoryFooter);

  char* imageViewString = malloc(n + strlen(imageViewFooter));
  strcpy(imageViewString, imageName);
  strcat(imageViewString, imageViewFooter);
  
  atlrSetObjectName(VK_OBJECT_TYPE_IMAGE, (AtlrU64)image->image, imageString, image->device);
  atlrSetObjectName(VK_OBJECT_TYPE_DEVICE_MEMORY, (AtlrU64)image->memory, memoryString, image->device);
  atlrSetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (AtlrU64)image->imageView, imageViewString, image->device);

  free(imageString);
  free(memoryString);
  free(imageViewString);
}
#endif

AtlrU8 atlrIsValidDepthImage(const AtlrImage* restrict image)
{
  for (AtlrU32 i = 0; i < sizeof(depthFormatChoices) / sizeof(VkFormat); i++)
    if (image->format == depthFormatChoices[i])
      return 1;
  return 0;
}
