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

#include "offscreen-canvas.h"
#include <stdio.h>

AtlrU8 atlrInitOffscreenCanvas(AtlrOffscreenCanvas* restrict canvas, const VkExtent2D* restrict extent, const VkFormat colorFormat, const AtlrU8 initRenderPass, const VkClearValue* restrict clearColor,
			       const AtlrDevice* restrict device)
{
  canvas->device = device;
  canvas->extent = *extent;
  VkClearValue clearValue = clearColor ? *clearColor : (VkClearValue){.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}};
  
  const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  const VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;

  // color image
  const VkImageUsageFlags colorUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  const VkImageAspectFlags colorAspect =  VK_IMAGE_ASPECT_COLOR_BIT;
  if (!atlrInitImage(&canvas->colorImage, extent->width, extent->height, 1, VK_SAMPLE_COUNT_1_BIT, colorFormat, tiling, colorUsage, memoryProperties, viewType, colorAspect, device))
  {
    ATLR_ERROR_MSG("atlrInitImage returned 0.");
    return 0;
  }
#ifdef ATLR_DEBUG
  atlrSetImageName(&canvas->colorImage, "Offscreen Canvas Framebuffer Color Image");
#endif

  // depth image
  const VkFormat depthFormat = atlrGetSupportedDepthImageFormat(device->physical, tiling);
  if (depthFormat == VK_FORMAT_UNDEFINED)
  {
    ATLR_ERROR_MSG("atlrGetSupportedDepthImageFormat returned VK_FORMAT_UNDEFINED.");
    return 0;
  }
  const VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  const VkImageAspectFlags depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  if (!atlrInitImage(&canvas->depthImage, extent->width, extent->height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, tiling, depthUsage, memoryProperties, viewType, depthAspect, device))
  {
    ATLR_ERROR_MSG("atlrInitImage returned 0.");
    return 0;
  }
#ifdef ATLR_DEBUG
  atlrSetImageName(&canvas->depthImage, "Offscreen Canvas Framebuffer Depth Image");
#endif

  if (initRenderPass)
  {
    const VkAttachmentDescription colorAttachment = atlrGetColorAttachmentDescription(colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    const VkAttachmentDescription depthAttachment = atlrGetDepthAttachmentDescription(VK_SAMPLE_COUNT_1_BIT, device, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    const VkSubpassDependency dependency =
    {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      .dependencyFlags = 0
    };
    if (!atlrInitRenderPass(&canvas->renderPass, 1, &colorAttachment, NULL, &clearValue, &depthAttachment, 1, &dependency, device))
    {
      ATLR_ERROR_MSG("atlrInitRenderPass returned 0.");
      return 0;
    }
#ifdef ATLR_DEBUG
    atlrSetRenderPassName(&canvas->renderPass, "Offsceen Canvas Render Pass");
#endif
  }

  const VkImageView framebufferAttachments[2] =
  {
    canvas->colorImage.imageView,
    canvas->depthImage.imageView
  };
  const VkFramebufferCreateInfo framebufferInfo =
  {
    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .renderPass = canvas->renderPass.renderPass,
    .attachmentCount = 2,
    .pAttachments = framebufferAttachments,
    .width = extent->width,
    .height = extent->height,
    .layers = 1
  };
  if (vkCreateFramebuffer(device->logical, &framebufferInfo, device->instance->allocator, &canvas->framebuffer) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateFrameBuffer did not return VK_SUCCESS.");
    return 0;
  }
#ifdef ATLR_DEBUG
    char framebufferString[64];
    sprintf(framebufferString, "Offsceen Canvas Framebuffer");
    atlrSetObjectName(VK_OBJECT_TYPE_FRAMEBUFFER, (AtlrU64)canvas->framebuffer, framebufferString, device);
#endif

  return 1;
}

void atlrDeinitOffscreenCanvas(const AtlrOffscreenCanvas* restrict canvas, const AtlrU8 deinitRenderPass)
{
  const AtlrDevice* device = canvas->device;
  vkDestroyFramebuffer(device->logical, canvas->framebuffer, device->instance->allocator);
  if (deinitRenderPass) atlrDeinitRenderPass(&canvas->renderPass);
  atlrDeinitImage(&canvas->depthImage);
  atlrDeinitImage(&canvas->colorImage);
}
