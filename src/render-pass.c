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

static const VkClearValue clearColor =
{
  .color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}
};
static const VkClearValue clearDepth =
{
  .depthStencil = {.depth = 0.0f, .stencil = 0} // for the reverse-z convention, depth should be 0.0f
};

VkAttachmentDescription atlrGetColorAttachmentDescription(const VkFormat format, const VkImageLayout finalLayout)
{
  return (VkAttachmentDescription)
    {
      .format = format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, 
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = finalLayout
    };
}

VkAttachmentDescription atlrGetDepthAttachmentDescription(const AtlrImage* restrict image)
{
  VkFormat format = VK_FORMAT_UNDEFINED;
  if (!atlrIsValidDepthImage(image))
    ATLR_ERROR_MSG("atlrIsValidDepthImage returned 0.");
  else
    format = image->format;

  return (VkAttachmentDescription)
    {
      .format = format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, 
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
}

AtlrU8 atlrInitRenderPass(AtlrRenderPass* renderPass,
			  const AtlrU32 colorAttachmentCount, const VkAttachmentDescription* restrict colorAttachments,
			  const VkAttachmentDescription* restrict depthAttachment,
			  const AtlrDevice* restrict device)
{
  renderPass->device = device;
  
  AtlrU32 attachmentCount = colorAttachmentCount;
  if (depthAttachment)
    attachmentCount++;

  VkAttachmentDescription* attachments = malloc(attachmentCount * sizeof(VkAttachmentDescription));
  memcpy(attachments, colorAttachments, colorAttachmentCount * sizeof(VkAttachmentDescription));
  if (depthAttachment)
    memcpy(attachments + colorAttachmentCount, depthAttachment, sizeof(VkAttachmentDescription));

  VkAttachmentReference* references = malloc(attachmentCount * sizeof(VkAttachmentReference));
  for (AtlrU32 i = 0; i < colorAttachmentCount; i++)
    references[i] = (VkAttachmentReference)
    {
      .attachment = i,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
  if (depthAttachment)
    references[colorAttachmentCount] = (VkAttachmentReference)
    {
      .attachment = colorAttachmentCount,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };	 

  const VkSubpassDescription subpass =
  {
    .flags = 0,
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .inputAttachmentCount = 0,
    .pInputAttachments = NULL,
    .colorAttachmentCount = colorAttachmentCount,
    .pColorAttachments = references,
    .pResolveAttachments = NULL,
    .pDepthStencilAttachment = references + colorAttachmentCount,
    .preserveAttachmentCount = 0,
    .pPreserveAttachments = NULL
  };

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

  const VkRenderPassCreateInfo renderPassInfo =
  {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext = NULL,
    .attachmentCount = attachmentCount,
    .pAttachments = attachments,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency
  };
  if (vkCreateRenderPass(device->logical, &renderPassInfo, device->instance->allocator, &renderPass->renderPass) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateRenderPass did not return VK_SUCCESS.");
    free(references);
    free(attachments);
    return 0;
  }

  VkClearValue* clearValues = malloc(attachmentCount * sizeof(VkClearValue));
  for (AtlrU32 i = 0; i < colorAttachmentCount; i++)
    clearValues[i] = clearColor;
  if (depthAttachment)
    clearValues[colorAttachmentCount] = clearDepth;
  renderPass->clearValueCount = attachmentCount;
  renderPass->clearValues = clearValues;
  
  free(references);
  free(attachments);
  return 1;
}

void atlrDeinitRenderPass(const AtlrRenderPass* restrict renderPass)
{
  free(renderPass->clearValues);
  const AtlrDevice* device = renderPass->device;
  vkDestroyRenderPass(device->logical, renderPass->renderPass, device->instance->allocator);
}

void atlrBeginRenderPass(const AtlrRenderPass* restrict renderPass,
			 const VkCommandBuffer commandBuffer, const VkFramebuffer framebuffer, const VkExtent2D* restrict extent)
{
  const VkRenderPassBeginInfo beginInfo =
  {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .pNext = NULL,
    .renderPass = renderPass->renderPass,
    .framebuffer = framebuffer,
    .renderArea = (VkRect2D)
    {
      .offset = {0, 0},
      .extent = *extent
    },
    .clearValueCount = renderPass->clearValueCount,
    .pClearValues = renderPass->clearValues
  };
  vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void atlrEndRenderPass(const VkCommandBuffer commandBuffer)
{
  vkCmdEndRenderPass(commandBuffer);
}
