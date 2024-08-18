#pragma once
#include "antler.h"

typedef struct _AtlrOffscreenCanvas
{
  const AtlrDevice* device;
  VkExtent2D extent;
  AtlrImage colorImage;
  AtlrImage depthImage;
  AtlrRenderPass renderPass;
  VkFramebuffer framebuffer;
  
} AtlrOffscreenCanvas;

AtlrU8 atlrInitOffscreenCanvas(AtlrOffscreenCanvas* restrict, const VkExtent2D* restrict extent, const VkFormat colorFormat, const AtlrU8 initRenderPass, const VkClearValue* restrict clearColor,
				const AtlrDevice* restrict);
void atlrDeinitOffscreenCanvas(const AtlrOffscreenCanvas* restrict, const AtlrU8 deinitRenderPass);
static inline void atlrOffscreenCanvasBeginRenderPass(const AtlrOffscreenCanvas* restrict canvas, const VkCommandBuffer commandBuffer)
{
  const VkExtent2D* extent = &canvas->extent;
  atlrBeginRenderPass(&canvas->renderPass, commandBuffer, canvas->framebuffer, extent);
  atlrCommandSetViewport(commandBuffer, extent->width, extent->height);
  atlrCommandSetScissor(commandBuffer, extent);
}
static inline void atlrOffscreenCanvasEndRenderPass(const VkCommandBuffer commandBuffer)
{
  atlrEndRenderPass(commandBuffer);
}
