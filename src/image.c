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

AtlrU8 atlrInitImageView(VkImageView* restrict imageView, const VkImage image, const VkImageViewType t, const VkFormat format, const VkImageAspectFlags aspectFlags, const AtlrU32 layerCount,
			 const AtlrDevice* restrict device)
{
  const VkImageViewCreateInfo imageViewInfo =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .image = image,
    .viewType = t,
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
