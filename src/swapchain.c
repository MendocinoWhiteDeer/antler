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

static VkSurfaceFormatKHR getSurfaceFormat(const VkSurfaceFormatKHR* restrict availableFormats, const AtlrU32 availableFormatCount)
{
  for (AtlrU32 i = 0; i < availableFormatCount; i++)
  {
    const VkSurfaceFormatKHR* format = availableFormats + i;
    if (format->format == VK_FORMAT_B8G8R8A8_SRGB && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return *format;
  }

  return availableFormats[0];
}

static VkPresentModeKHR getPresetMode(const VkPresentModeKHR* restrict availablePresetModes, const AtlrU32 availablePresetModesCount)
{
  for (AtlrU32 i = 0; i < availablePresetModesCount; i++)
  {
    VkPresentModeKHR presentMode = availablePresetModes[i];
    if (availablePresetModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
      return presentMode;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D getExtent(const VkSurfaceCapabilitiesKHR* restrict capabilities, const AtlrInstance* instance)
{
  if (capabilities->currentExtent.width != UINT32_MAX)
    return capabilities->currentExtent;

  int width, height;
  GLFWwindow* window = instance->data;
  glfwGetFramebufferSize(window, &width, &height);
  VkExtent2D extent =
  {
    .width = width,
    .height = height
  };
  const VkExtent2D min = capabilities->minImageExtent;
  const VkExtent2D max = capabilities->maxImageExtent;
  if (extent.width < min.width)
    extent.width = min.width;
  else if (extent.width > max.width)
    extent.width = max.width;
  if (extent.height < min.height)
    extent.height = min.height;
  else if (extent.height > max.height)
    extent.height = max.height;

  return extent;
}

AtlrU8 atlrInitSwapchainHostGLFW(AtlrSwapchain* restrict swapchain, const AtlrU8 initRenderPass,
				 const AtlrDevice* restrict device)
{
  if (device->instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_ERROR_MSG("Antler is not in host GLFW mode.");
    return 0;
  } 
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device->physical, &properties);
  if (!device->queueFamilyIndices.isGraphicsCompute || !device->queueFamilyIndices.isPresent)
  {
    ATLR_ERROR_MSG("The AtlrDevice with handle \"%s\" lacks queue family support.", properties.deviceName);
    return 0;
  }
  if (!device->hasSwapchainSupport)
  {
    ATLR_ERROR_MSG("The AtlrDevice with handle \"%s\" lacks swapchain support.", properties.deviceName);
    return 0;
  }
  atlrLog(ATLR_LOG_INFO, "Initializing Antler swapchain in host GLFW mode ...");
  swapchain->device = device;

  // support details need to be initialized whenever the swap chain is (re)created, the support details can change on window resize
  AtlrSwapchainSupportDetails supportDetails;
  atlrInitSwapchainSupportDetails(&supportDetails, device->instance, device->physical);
  const VkExtent2D extent = getExtent(&supportDetails.capabilities, device->instance);
  const VkSurfaceFormatKHR surfaceFormat = getSurfaceFormat(supportDetails.formats, supportDetails.formatCount);
  const VkPresentModeKHR presentMode = getPresetMode(supportDetails.presentModes, supportDetails.presentModeCount);

  // determine the image count, when the max count is zero it pretty much means an unrestricted amount of images is allowed
  const AtlrU32 maxImageCount = supportDetails.capabilities.maxImageCount;
  AtlrU32 imageCount = supportDetails.capabilities.minImageCount + 1; // add 1 to min to prevent waiting
  if (maxImageCount && (imageCount > maxImageCount))
    imageCount = maxImageCount;

  VkSwapchainCreateInfoKHR swapchainInfo =
  {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .surface = device->instance->surface,
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = supportDetails.capabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = presentMode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE
  };

  // determine if image is shared across queue families
  const AtlrQueueFamilyIndices* indices = &device->queueFamilyIndices; 
  AtlrU32 indicesArr[2] =
  {
    indices->graphicsComputeIndex,
    indices->presentIndex
  };
  if (indices->graphicsComputeIndex == indices->presentIndex)
  {
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = NULL;
  }
  else
  {
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainInfo.queueFamilyIndexCount = 2;
    swapchainInfo.pQueueFamilyIndices = indicesArr;
  }

  if (vkCreateSwapchainKHR(device->logical, &swapchainInfo, device->instance->allocator, &swapchain->swapchain) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateSwapchainKHR did not return VK_SUCCESS.");
    return 0;
  }
  swapchain->format = surfaceFormat.format;
  swapchain->extent = extent;
  atlrDeinitSwapchainSupportDetails(&supportDetails);

  if (vkGetSwapchainImagesKHR(device->logical, swapchain->swapchain, &imageCount, NULL) != VK_SUCCESS)
  {
    atlrLog(ATLR_LOG_ERROR, "vkCreateSwapchainKHR (first call) did not return VK_SUCCESS.");
    return 0;
  }
  VkImage* images = malloc(imageCount * sizeof(VkImage));
  if (vkGetSwapchainImagesKHR(device->logical, swapchain->swapchain, &imageCount, images) != VK_SUCCESS)
  {
    atlrLog(ATLR_LOG_ERROR, "vkCreateSwapchainKHR (second call) did not return VK_SUCCESS.");
    return 0;
  }
  swapchain->imageCount = imageCount;
  swapchain->images = images;
  
  VkImageView* imageViews = malloc(imageCount * sizeof(VkImageView));
  for (AtlrU32 i = 0; i < imageCount; i++)
  {
    VkImageView imageView = atlrInitImageView(images[i], VK_IMAGE_VIEW_TYPE_2D, surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, device);
    if (imageView == VK_NULL_HANDLE)
    {
      ATLR_ERROR_MSG("atlrInitImageView returned VK_NULL_HANDLE.");
      return 0;
    }
    imageViews[i] = imageView;
  }
  swapchain->imageViews = imageViews;

  if (!atlrInitDepthImage(&swapchain->depthImage, extent.width, extent.height, device))
  {
    ATLR_ERROR_MSG("atlrInitDepthImage returned 0.");
    return 0;
  }

  if (initRenderPass)
  {
    const VkAttachmentDescription colorAttachment = atlrGetColorAttachmentDescription(swapchain->format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    const VkAttachmentDescription depthAttachment = atlrGetDepthAttachmentDescription(&swapchain->depthImage);
    if (!atlrInitRenderPass(&swapchain->renderPass, 1, &colorAttachment, &depthAttachment, device))
    {
      ATLR_ERROR_MSG("atlrInitRenderPass returned 0.");
      return 0;
    }
  }

  VkFramebuffer* framebuffers = malloc(imageCount * sizeof(VkFramebuffer));
  for (AtlrU32 i = 0; i < imageCount; i++)
  {
    const VkImageView framebufferAttachments[2] =
    {
      imageViews[i],
      swapchain->depthImage.imageView
    };
    const VkFramebufferCreateInfo framebufferInfo =
    {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .renderPass = swapchain->renderPass.renderPass,
      .attachmentCount = 2,
      .pAttachments = framebufferAttachments,
      .width = extent.width,
      .height = extent.height,
      .layers = 1
    };
    if (vkCreateFramebuffer(device->logical, &framebufferInfo, device->instance->allocator, framebuffers + i) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkCreateFramebuffer did not return VK_SUCCESS.");
      return 0;
    }
  }
  swapchain->framebuffers = framebuffers;


  atlrLog(ATLR_LOG_INFO, "Done initializing Antler swapchain.");
  return 1;
}

void atlrDeinitSwapchainHostGLFW(AtlrSwapchain* restrict swapchain, const AtlrU8 deinitRenderPass)
{ 
  const AtlrDevice* device = swapchain->device;
  if (device->instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_ERROR_MSG("Antler is not in host GLFW mode.");
    return;
  }
  atlrLog(ATLR_LOG_INFO, "Deinitializing Antler swapchain in host GLFW mode ...");

  for (AtlrU32 i = 0; i < swapchain->imageCount; i++)
    vkDestroyFramebuffer(device->logical, swapchain->framebuffers[i], device->instance->allocator);

  if (deinitRenderPass)
    atlrDeinitRenderPass(&swapchain->renderPass);

  atlrDeinitImage(&swapchain->depthImage);
  
  for (AtlrU32 i = 0; i < swapchain->imageCount; i++)
    atlrDeinitImageView(swapchain->imageViews[i], device);
  free(swapchain->imageViews);
  free(swapchain->images);
  vkDestroySwapchainKHR(device->logical, swapchain->swapchain, device->instance->allocator);
  atlrLog(ATLR_LOG_INFO, "Done deinitializing Antler swapchain.");
}

AtlrU8 atlrReinitSwapchainHostGLFW(AtlrSwapchain* restrict swapchain)
{
  const AtlrDevice* device = swapchain->device;
  
  if (device->instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_ERROR_MSG("Antler is not in host GLFW mode.");
    return 0;
  }
  
  int width = 0;
  int height = 0;
  GLFWwindow* window = device->instance->data;
  glfwGetFramebufferSize(window, &width, &height);
  while (!width || !height)
  {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }
  
  vkDeviceWaitIdle(device->logical);
  
  atlrDeinitSwapchainHostGLFW(swapchain, 0);
  if (!atlrInitSwapchainHostGLFW(swapchain, 0, device))
  {
    ATLR_ERROR_MSG("atlrInitSwapchainHostGLFW returned 0.");
    return 0;
  }

  return 1;
}

VkResult atlrNextSwapchainImage(const AtlrSwapchain* restrict swapchain, const VkSemaphore imageAvailableSemaphore, AtlrU32* imageIndex)
{
  return vkAcquireNextImageKHR(swapchain->device->logical, swapchain->swapchain, UINT64_MAX,
			       imageAvailableSemaphore, VK_NULL_HANDLE, imageIndex);
}

VkResult atlrSwapchainSubmit(const AtlrSwapchain* restrict swapchain, const VkCommandBuffer commandBuffer,
			     const VkSemaphore imageAvailableSemaphore, const VkSemaphore renderFinishedSemaphore, const VkFence fence)
{
  const VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  const VkSubmitInfo submitInfo =
  {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = NULL,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &imageAvailableSemaphore,
    .pWaitDstStageMask = &submitWaitStage,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &renderFinishedSemaphore
  };
  
  return vkQueueSubmit(swapchain->device->graphicsComputeQueue, 1, &submitInfo, fence);
}

VkResult atlrSwapchainPresent(const AtlrSwapchain* restrict swapchain, const VkSemaphore renderFinishedSemaphore, const AtlrU32* restrict imageIndex)
{
  const VkPresentInfoKHR presentInfo =
  {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = NULL,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &renderFinishedSemaphore,
    .swapchainCount = 1,
    .pSwapchains = &swapchain->swapchain,
    .pImageIndices = imageIndex,
    .pResults = NULL
  };
  
  return vkQueuePresentKHR(swapchain->device->presentQueue, &presentInfo);
}				    
