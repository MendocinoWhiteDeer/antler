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

static const VkClearValue clearDepth =
{
  .depthStencil = {.depth = 0.0f, .stencil = 0} // for the reverse-z convention, depth should be 0.0f
};

VkAttachmentDescription atlrGetColorAttachmentDescription(const VkFormat format, const VkSampleCountFlagBits samples, const VkImageLayout finalLayout)
{
  return (VkAttachmentDescription)
    {
      .format = format,
      .samples = samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, 
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = finalLayout
    };
}

VkAttachmentDescription atlrGetDepthAttachmentDescription(const VkSampleCountFlagBits samples, const AtlrDevice* device, const VkImageLayout finalLayout)
{
  const VkImageTiling dphImgTiling = VK_IMAGE_TILING_OPTIMAL;
  const VkFormat format = atlrGetSupportedDepthImageFormat(device->physical, dphImgTiling);
  if (format == VK_FORMAT_UNDEFINED)
    ATLR_ERROR_MSG("atlrGetSupportedDepthImageFormat returned VK_NULL_HANDLE.");

  return (VkAttachmentDescription)
    {
      .format = format,
      .samples = samples,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, 
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = finalLayout
    };
}

AtlrU8 atlrInitRenderPass(AtlrRenderPass* renderPass,
			  const AtlrU32 colorAttachmentCount, const VkAttachmentDescription* restrict colorAttachments, const VkAttachmentDescription* restrict resolveAttachments, const VkClearValue* restrict clearColor,
			  const VkAttachmentDescription* restrict depthAttachment,
			  const AtlrU32 dependencyCount, const VkSubpassDependency* restrict dependencies,
			  const AtlrDevice* restrict device)
{
  renderPass->device = device;
  
  AtlrU32 attachmentCount = colorAttachmentCount;
  if (depthAttachment) attachmentCount++;
  if (resolveAttachments) attachmentCount += colorAttachmentCount;

  VkAttachmentDescription* attachments = malloc(attachmentCount * sizeof(VkAttachmentDescription));
  memcpy(attachments, colorAttachments, colorAttachmentCount * sizeof(VkAttachmentDescription));
  if (depthAttachment)
    memcpy(attachments + colorAttachmentCount, depthAttachment, sizeof(VkAttachmentDescription));
  if (resolveAttachments)
    memcpy(attachments + colorAttachmentCount + 1, resolveAttachments, colorAttachmentCount * sizeof(VkAttachmentDescription));

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
  if (resolveAttachments)
    for (AtlrU32 i = 0; i < colorAttachmentCount; i++)
      references[colorAttachmentCount + 1 + i] = (VkAttachmentReference)
      {
	.attachment = colorAttachmentCount + 1 + i,
	.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      };

  const VkSubpassDescription subpass =
  {
    .flags = 0,
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .inputAttachmentCount = 0,
    .pInputAttachments = NULL,
    .colorAttachmentCount = colorAttachmentCount,
    .pColorAttachments = references,
    .pResolveAttachments = resolveAttachments ? references + 1 + colorAttachmentCount : NULL,
    .pDepthStencilAttachment = references + colorAttachmentCount,
    .preserveAttachmentCount = 0,
    .pPreserveAttachments = NULL
  };

  const VkRenderPassCreateInfo renderPassInfo =
  {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext = NULL,
    .attachmentCount = attachmentCount,
    .pAttachments = attachments,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = dependencyCount,
    .pDependencies = dependencies
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
    clearValues[i] = *clearColor;
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

#ifdef ATLR_DEBUG
void atlrSetRenderPassName(const AtlrRenderPass* restrict renderPass, const char* restrict renderPassName)
{
  atlrSetObjectName(VK_OBJECT_TYPE_RENDER_PASS, (AtlrU64)renderPass->renderPass, renderPassName, renderPass->device); 
}
#endif

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
