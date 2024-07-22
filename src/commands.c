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

AtlrU8 atlrInitGraphicsCommandPool(VkCommandPool* restrict commandPool, const VkCommandPoolCreateFlags flags,
				   const AtlrInstance* restrict instance, const AtlrDevice* restrict device)
{
  const VkCommandPoolCreateInfo poolInfo =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = flags,
    .queueFamilyIndex = device->queueFamilyIndices.graphics_index
  };
  if (vkCreateCommandPool(device->logical, &poolInfo, instance->allocator, commandPool) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_ERROR, "vkCreateCommandPool did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

void atlrDeinitCommandPool(const VkCommandPool commandPool,
			   const AtlrInstance* restrict instance, const AtlrDevice* restrict device)
{
  vkDestroyCommandPool(device->logical, commandPool, instance->allocator);
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
    atlrLogMsg(LOG_ERROR, "vkAllocateCommandBuffers did not return VK_SUCCESS.");
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
    atlrLogMsg(LOG_ERROR, "vkBeginCommandBuffer did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrEndCommandRecording(const VkCommandBuffer commandBuffer)
{
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_ERROR, "vkEndCommandBuffer did not return VK_SUCCESS.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrInitSingleRecordCommandContext(AtlrSingleRecordCommandContext* restrict commandContext,
					  const AtlrInstance* restrict instance, const AtlrDevice* restrict device)
{
  if (!atlrInitGraphicsCommandPool(&commandContext->commandPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
				   instance, device))
  {
    atlrLogMsg(LOG_ERROR, "atlrInitGraphicsCommandPool returned 0.");
    return 0;
  }

  const VkFenceCreateInfo fenceInfo =
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0
  };
  if (vkCreateFence(device->logical, &fenceInfo, instance->allocator, &commandContext->fence) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_ERROR, "vkCreateFence did not return VK_SUCCESS.");
      return 0;
    }

  return 1;
}

void atlrDeinitSingleRecordCommandContext(AtlrSingleRecordCommandContext* restrict commandContext,
					  const AtlrInstance* restrict instance, const AtlrDevice* restrict device)
{
  vkDestroyFence(device->logical, commandContext->fence, instance->allocator);
  atlrDeinitCommandPool(commandContext->commandPool, instance, device);
}

AtlrU8 atlrBeginSingleRecordCommands(VkCommandBuffer* restrict commandBuffer, const AtlrSingleRecordCommandContext* restrict commandContext,
				     const AtlrDevice* device)
{
  if (!atlrAllocatePrimaryCommandBuffers(commandBuffer, 1, commandContext->commandPool, device))
  {
    atlrLogMsg(LOG_ERROR, "atlrAllocatePrimaryCommandBuffers returned 0.");
    return 0;
  }
  
  if (!atlrBeginCommandRecording(*commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
  {
    atlrLogMsg(LOG_ERROR, "altrBeginCommandRecording returned 0.");
    return 0;
  }

  return 1;
}

AtlrU8 atlrEndSingleRecordCommands(const VkCommandBuffer commandBuffer, const AtlrSingleRecordCommandContext* restrict commandContext,
				   const AtlrDevice* device)
{
  if (!atlrEndCommandRecording(commandBuffer))
  {
    atlrLogMsg(LOG_ERROR, "atlrEndCommandRecording returned 0.");
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
  if (vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_ERROR, "vkQueueSubmit did not return VK_SUCCESS.");
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
