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

static void windowResizeCallback(GLFWwindow* window, int width, int height)
{
  AtlrFrameCommandContext* commandContext = (AtlrFrameCommandContext*)glfwGetWindowUserPointer(window);
  commandContext->isResize = 1;
}

AtlrU8 atlrInitCommandPool(VkCommandPool* restrict commandPool, const VkCommandPoolCreateFlags flags, const AtlrU32 queueFamilyIndex,
				   const AtlrDevice* restrict device)
{
  const VkCommandPoolCreateInfo poolInfo =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = flags,
    .queueFamilyIndex = queueFamilyIndex
  };
  if (vkCreateCommandPool(device->logical, &poolInfo, device->instance->allocator, commandPool) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateCommandPool did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

void atlrDeinitCommandPool(const VkCommandPool commandPool,
			   const AtlrDevice* restrict device)
{
  vkDestroyCommandPool(device->logical, commandPool, device->instance->allocator);
}

AtlrU8 atlrAllocatePrimaryCommandBuffers(VkCommandBuffer* restrict commandBuffers, AtlrU32 commandBufferCount, const VkCommandPool commandPool,
					 const AtlrDevice* device)
{
  const VkCommandBufferAllocateInfo allocInfo =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = NULL,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = commandBufferCount
  };
  if (vkAllocateCommandBuffers(device->logical, &allocInfo, commandBuffers) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkAllocateCommandBuffers did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrBeginCommandRecording(const VkCommandBuffer commandBuffer, const VkCommandBufferUsageFlags flags)
{
  const VkCommandBufferBeginInfo info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = NULL,
    .flags = flags,
    .pInheritanceInfo = NULL
  };
  if (vkBeginCommandBuffer(commandBuffer, &info) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkBeginCommandBuffer did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrEndCommandRecording(const VkCommandBuffer commandBuffer)
{
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkEndCommandBuffer did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrInitSingleRecordCommandContext(AtlrSingleRecordCommandContext* restrict commandContext, const AtlrU32 queueFamilyIndex,
					  const AtlrDevice* restrict device)
{
  commandContext->device = device;
  vkGetDeviceQueue(device->logical, queueFamilyIndex, 0, &commandContext->queue);
  
  if (!atlrInitCommandPool(&commandContext->commandPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndex, device))
  {
    ATLR_ERROR_MSG("atlrInitGraphicsCommandPool returned 0.");
    return 0;
  }

  const VkFenceCreateInfo fenceInfo =
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0
  };
  if (vkCreateFence(device->logical, &fenceInfo, device->instance->allocator, &commandContext->fence) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkCreateFence did not return VK_SUCCESS.");
      return 0;
    }

  return 1;
}

void atlrDeinitSingleRecordCommandContext(AtlrSingleRecordCommandContext* restrict commandContext)
{
  const AtlrDevice* device = commandContext->device;
  vkDestroyFence(device->logical, commandContext->fence, device->instance->allocator);
  atlrDeinitCommandPool(commandContext->commandPool, device);
}

AtlrU8 atlrBeginSingleRecordCommands(VkCommandBuffer* restrict commandBuffer, const AtlrSingleRecordCommandContext* restrict commandContext)
{
  const AtlrDevice* device = commandContext->device;
  if (!atlrAllocatePrimaryCommandBuffers(commandBuffer, 1, commandContext->commandPool, device))
  {
    ATLR_ERROR_MSG("atlrAllocatePrimaryCommandBuffers returned 0.");
    return 0;
  }
  
  if (!atlrBeginCommandRecording(*commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
  {
    ATLR_ERROR_MSG("altrBeginCommandRecording returned 0.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrEndSingleRecordCommands(const VkCommandBuffer commandBuffer, const AtlrSingleRecordCommandContext* restrict commandContext)
{
  const AtlrDevice* device = commandContext->device;
  if (!atlrEndCommandRecording(commandBuffer))
  {
    ATLR_ERROR_MSG("atlrEndCommandRecording returned 0.");
    return 0;
  }

  VkFence fence = commandContext->fence;
  const VkSubmitInfo submitInfo =
  {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = NULL,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = NULL,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
    .signalSemaphoreCount = 0,
    .pSignalSemaphores = NULL
  };
  if (vkQueueSubmit(commandContext->queue, 1, &submitInfo, fence) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkQueueSubmit did not return VK_SUCCESS.");
    return 0;
  }

  vkWaitForFences(device->logical, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device->logical, 1, &fence);

  vkFreeCommandBuffers(device->logical, commandContext->commandPool, 1, &commandBuffer);

  return 1;
}

void atlrCommandSetViewport(const VkCommandBuffer commandBuffer, const float width, const float height)
{
  const VkViewport viewport =
  {
    .x = 0.0f,
    .y = 0.0f,
    .width = width,
    .height = height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void atlrCommandSetScissor(const VkCommandBuffer commandBuffer, const VkExtent2D* extent)
{
  const VkRect2D scissor =
  {
    .offset = {0, 0},
    .extent = *extent
  };
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

AtlrU8 atlrInitFrameCommandContextHostGLFW(AtlrFrameCommandContext* restrict commandContext, const AtlrU8 frameCount,
				   AtlrSwapchain* restrict swapchain)
{ 
  commandContext->imageIndex = 0;
  commandContext->swapchain = swapchain;
  const AtlrDevice* device = swapchain->device;

  if (device->instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_ERROR_MSG("Antler is not in host GLFW mode.");
    return 0;
  } 

  GLFWwindow* window = device->instance->data;
  glfwSetWindowUserPointer(window, commandContext);
  glfwSetFramebufferSizeCallback(window, windowResizeCallback);

  if(!atlrInitCommandPool(&commandContext->commandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, device->queueFamilyIndices.graphicsComputeIndex, device))
  {
    ATLR_ERROR_MSG("atlrInitGraphicsCommandPool returned 0.");
    return 0;
  }

  AtlrFrame* frames = malloc(frameCount * sizeof(AtlrFrame));
  const VkSemaphoreCreateInfo semaphoreInfo =
  {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0
  };
  const VkFenceCreateInfo fenceInfo =
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = NULL,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };
  for (AtlrU8 i = 0; i < frameCount; i++)
  {
    AtlrFrame* frame = frames + i;
    if (!atlrAllocatePrimaryCommandBuffers(&frame->commandBuffer, 1, commandContext->commandPool, device))
    {
      ATLR_ERROR_MSG("atlrAllocatePrimaryCommandBuffers returned 0.");
      return 0;
    }
    if (vkCreateSemaphore(device->logical, &semaphoreInfo, device->instance->allocator, &frame->imageAvailableSemaphore) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkCreateSemaphore did not return VK_SUCCESS.");
      return 0;
    }
    if (vkCreateSemaphore(device->logical, &semaphoreInfo, device->instance->allocator, &frame->renderFinishedSemaphore) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkCreateSemaphore did not return VK_SUCCESS.");
      return 0;
    }
    if (vkCreateFence(device->logical, &fenceInfo, device->instance->allocator, &frame->inFlightFence) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkCreateFence did not return VK_SUCCESS.");
      return 0;
    }
  }
  commandContext->currentFrame = 0;
  commandContext->frameCount = frameCount;
  commandContext->frames = frames;

  return 1;
}

void atlrDeinitFrameCommandContextHostGLFW(AtlrFrameCommandContext* restrict commandContext)
{
  const AtlrDevice* device = commandContext->swapchain->device;

  if (device->instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_ERROR_MSG("Antler is not in host GLFW mode.");
    return;
  } 
  
  for (AtlrU8 i = 0; i < commandContext->frameCount; i++)
  {
    const AtlrFrame* frame = commandContext->frames + i;
    vkDestroyFence(device->logical, frame->inFlightFence, device->instance->allocator);
    vkDestroySemaphore(device->logical, frame->renderFinishedSemaphore, device->instance->allocator);
    vkDestroySemaphore(device->logical, frame->imageAvailableSemaphore, device->instance->allocator);
  }
  free(commandContext->frames);

  atlrDeinitCommandPool(commandContext->commandPool, device);
}

AtlrU8 atlrBeginFrameCommandsHostGLFW(AtlrFrameCommandContext* restrict commandContext)
{
  const AtlrFrame* frame = commandContext->frames + commandContext->currentFrame;
  AtlrSwapchain* swapchain = commandContext->swapchain;
  const AtlrDevice* device = swapchain->device;

  if (device->instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_ERROR_MSG("Antler is not in host GLFW mode.");
    return 0;
  } 
  
  vkWaitForFences(device->logical, 1, &frame->inFlightFence, VK_TRUE, UINT64_MAX);

  VkResult swapchainResult = atlrNextSwapchainImage(swapchain, frame->imageAvailableSemaphore, &commandContext->imageIndex);
  if (swapchainResult == VK_ERROR_OUT_OF_DATE_KHR)
  {
    if (!atlrReinitSwapchainHostGLFW(swapchain))
    {
      ATLR_ERROR_MSG("atlrReinitSwapchainHostGLFW returned 0.");
      return 0;
    }

    atlrLog(ATLR_LOG_INFO, "Acquire swapchain image: image out of date");

    return atlrBeginFrameCommandsHostGLFW(commandContext);
  }
  else if (swapchainResult != VK_SUCCESS && swapchainResult != VK_SUBOPTIMAL_KHR)
  {
    ATLR_ERROR_MSG("Swapchain result is neither VK_SUCCESS nor VK_SUBOPTIMAL_KHR.");
    return 0;
  }
  
  vkResetFences(device->logical, 1, &frame->inFlightFence);

  const VkCommandBuffer commandBuffer = frame->commandBuffer;
  const VkExtent2D* extent = &swapchain->extent;
  vkResetCommandBuffer(commandBuffer, 0);
  if (!atlrBeginCommandRecording(commandBuffer, 0))
  {
    ATLR_ERROR_MSG("atlrBeginCommandRecording returned 0.");
    return 0;
  }
  atlrCommandSetViewport(commandBuffer, extent->width, extent->height);
  atlrCommandSetScissor(commandBuffer, extent);

  const VkFramebuffer framebuffer = swapchain->framebuffers[commandContext->imageIndex];
  atlrBeginRenderPass(&swapchain->renderPass, commandBuffer, framebuffer, extent);

  return 1;
}

AtlrU8 atlrEndFrameCommandsHostGLFW(AtlrFrameCommandContext* restrict commandContext)
{
  const AtlrFrame* frame = commandContext->frames + commandContext->currentFrame;
  AtlrSwapchain* swapchain = commandContext->swapchain;
  const VkCommandBuffer commandBuffer = frame->commandBuffer;

  if (swapchain->device->instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_ERROR_MSG("Antler is not in host GLFW mode.");
    return 0;
  } 

  atlrEndRenderPass(commandBuffer);

  atlrEndCommandRecording(commandBuffer);

  if (atlrSwapchainSubmit(swapchain, commandBuffer,
			  frame->imageAvailableSemaphore, frame->renderFinishedSemaphore, frame->inFlightFence) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("atlrSwapchainSubmit did not return VK_SUCCESS.");
    return 0;
  }

  VkResult swapchainResult = atlrSwapchainPresent(swapchain, frame->renderFinishedSemaphore, &commandContext->imageIndex);
  if (swapchainResult == VK_ERROR_OUT_OF_DATE_KHR || swapchainResult == VK_SUBOPTIMAL_KHR || commandContext->isResize)
  {
    if (swapchainResult == VK_ERROR_OUT_OF_DATE_KHR)
	atlrLog(ATLR_LOG_INFO, "Present swapchain image: image out of date");
    else if (swapchainResult == VK_SUBOPTIMAL_KHR)
      atlrLog(ATLR_LOG_INFO, "Present swapchain image: suboptimal");
    
    commandContext->isResize = 0;
    if (!atlrReinitSwapchainHostGLFW(swapchain))
    {
      ATLR_ERROR_MSG("atlrReinitSwapchainHostGLFW returned 0.");
      return 0;
    }
  }
  else if (swapchainResult != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("Swapchain result is not VK_SUCCESS.");
    return 0;
  }

  commandContext->currentFrame = (commandContext->currentFrame + 1) % commandContext->frameCount;

  return 1;
}

VkCommandBuffer atlrGetFrameCommandContextCommandBufferHostGLFW(const AtlrFrameCommandContext* restrict commandContext)
{
  if (commandContext->swapchain->device->instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_ERROR_MSG("Antler is not in host GLFW mode.");
    return VK_NULL_HANDLE;
  } 
  const AtlrFrame* frame = commandContext->frames + commandContext->currentFrame;
  return frame->commandBuffer;
}
