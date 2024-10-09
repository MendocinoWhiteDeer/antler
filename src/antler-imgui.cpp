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

/*
The code in this file was largly transcribed from something Sascha Willems wrote into my own functions.
https://github.com/SaschaWillems/Vulkan/blob/master/examples/imgui/main.cpp, which is under the MIT License.

Copyright 2017-2024 by Sascha Willems - www.saschawillems.de

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "antler-imgui.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <stdexcept>
#include <algorithm>

void Atlr::ImguiContext::init(const AtlrU8 frameCount, const AtlrSwapchain* restrict swapchain, const AtlrSingleRecordCommandContext* restrict commandContext)
{
  this->device = swapchain->device;
  
  ImGui::CreateContext();
  GLFWwindow* window = (GLFWwindow*)this->device->instance->data;
  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGuiIO& io = ImGui::GetIO();

  unsigned char* pixels;
  int fontImageWidth, fontImageHeight;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &fontImageWidth, &fontImageHeight);
  const AtlrU64 fontImageSize = 4 * fontImageWidth * fontImageHeight * sizeof(char);

  const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  const VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  const VkMemoryPropertyFlags memoryProperties =  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (!atlrInitImage(&this->fontImage, fontImageWidth, fontImageHeight, 1, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, usage, memoryProperties, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, device))
  {
    throw std::runtime_error("atlrInitImage returned 0.");
    return;
  }

  AtlrBuffer stagingBuffer;
  if (!atlrInitStagingBuffer(&stagingBuffer, fontImageSize, device))
  {
    throw std::runtime_error("atlrInitStagingBuffer returned 0.");
    return;
  }

  if (!atlrWriteBuffer(&stagingBuffer, 0, fontImageSize, 0, pixels))
  {
    throw std::runtime_error("atlrWriteBuffer returned 0.");
    return;
  }

  const VkImageLayout initLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
  const VkImageLayout secondLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  const VkImageLayout finalLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  const VkOffset2D offset          = {.x = 0, .y = 0};
  const VkExtent2D extent          = {.width = (AtlrU32)fontImageWidth, .height = (AtlrU32)fontImageHeight};
  if (!atlrTransitionImageLayout(&this->fontImage, initLayout, secondLayout, commandContext) ||
      !atlrCopyBufferToImage(&stagingBuffer, &this->fontImage, &offset, &extent, commandContext) ||
      !atlrTransitionImageLayout(&this->fontImage, secondLayout, finalLayout, commandContext))
  {
    std::runtime_error("Failed to stage texture image.");
    return;
  }

  atlrDeinitBuffer(&stagingBuffer);

  VkSamplerCreateInfo samplerInfo =
  {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .mipLodBias = 0.0f,
    .anisotropyEnable = VK_FALSE,
    .maxAnisotropy = 1.0f,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .minLod = 0.0f,
    .maxLod = 0.0f,
    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    .unnormalizedCoordinates = VK_FALSE
  };
  if (vkCreateSampler(device->logical, &samplerInfo, device->instance->allocator, &this->fontSampler) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateSampler did not return VK_SUCCESS.");
    return;
  }

  const VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    
  VkDescriptorSetLayoutBinding setLayoutBinding = atlrInitDescriptorSetLayoutBinding(0, descriptorType, VK_SHADER_STAGE_FRAGMENT_BIT);
  if (!atlrInitDescriptorSetLayout(&this->descriptorSetLayout, 1, &setLayoutBinding, device))
  {
    throw std::runtime_error("atlrInitDescriptorSetLayout returned 0.");
    return;
  }

  VkDescriptorPoolSize poolSize = atlrInitDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, frameCount);
  if (!atlrInitDescriptorPool(&this->descriptorPool, frameCount, 1, &poolSize, device))
  {
    throw std::runtime_error("atlrInitDescriptorPool returned 0.");
    return;
  }

  this->descriptorSets.resize(frameCount);

  std::vector<VkDescriptorSetLayout> setLayouts;
  setLayouts.resize(frameCount, this->descriptorSetLayout.layout);
  if (!atlrAllocDescriptorSets(&this->descriptorPool, frameCount, setLayouts.data(), this->descriptorSets.data()))
  {
    throw std::runtime_error("atlrAllocDescriptorSets returned 0.");
    return;
  }

  const VkDescriptorImageInfo imageInfo = atlrInitDescriptorImageInfo(&this->fontImage, this->fontSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  std::vector<VkWriteDescriptorSet> descriptorWrites;
  descriptorWrites.resize(frameCount);
  for (AtlrU8 i = 0; i < frameCount; i++)
    descriptorWrites[i] = atlrWriteImageDescriptorSet(descriptorSets[i], 0, descriptorType, &imageInfo);
  vkUpdateDescriptorSets(device->logical, frameCount, descriptorWrites.data(), 0, NULL);

  const char* vertexShaderSource =
    "#version 460\n"
    "layout (location = 0) in vec2 inPos;\n"
    "layout (location = 1) in vec2 inUv;\n"
    "layout (location = 2) in vec4 inColor;\n"
    "layout (push_constant) uniform Transform { vec2 translate; vec2 scale; } transform;\n"
    "layout (location = 0) out vec2 outUv;\n"
    "layout (location = 1) out vec4 outColor;\n"
    "void main() { outUv = inUv; outColor = inColor; gl_Position = vec4(transform.translate + transform.scale * inPos, 0.0f, 1.0f); }";
  VkShaderModule vertexModule;
  {
    AtlrSpirVBinary bin = {};
    if (!atlrInitSpirVBinary(&bin, GLSLANG_STAGE_VERTEX, vertexShaderSource, "vertex"))
    {
      throw std::runtime_error("atlrCompileShader returned 0.");
      return;
    }

    const VkShaderModuleCreateInfo moduleInfo =
    {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = bin.codeSize,
      .pCode = bin.code
    };
    if (vkCreateShaderModule(device->logical, &moduleInfo, device->instance->allocator, &vertexModule) != VK_SUCCESS)
    {
      throw std::runtime_error("vkCreateShaderModule did not return VK_SUCCESS.");
      return;
    }

    atlrDeinitSpirVBinary(&bin);
  }
  
  const char* fragmentShaderSource =
    "#version 460\n"
    "layout (location = 0) in vec2 inUv;\n"
    "layout (location = 1) in vec4 inColor;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "layout (set = 0, binding = 0) uniform sampler2D textureSampler;\n"
    "void main() { outColor = inColor * texture(textureSampler, inUv); }";
  VkShaderModule fragmentModule;
  {
    AtlrSpirVBinary bin = {};
    if (!atlrInitSpirVBinary(&bin, GLSLANG_STAGE_FRAGMENT, fragmentShaderSource, "fragment"))
    {
      throw std::runtime_error("atlrCompileShader returned 0.");
      return;
    }

    const VkShaderModuleCreateInfo moduleInfo =
    {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = bin.codeSize,
      .pCode = bin.code
    };
    if (vkCreateShaderModule(device->logical, &moduleInfo, device->instance->allocator, &fragmentModule) != VK_SUCCESS)
    {
      throw std::runtime_error("vkCreateShaderModule did not return VK_SUCCESS.");
      return;
    }

    atlrDeinitSpirVBinary(&bin);
  }
  
  VkPipelineShaderStageCreateInfo stageInfos[2] =
  {
    atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexModule),
    atlrInitPipelineShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentModule)
  };

  const VkVertexInputBindingDescription vertexInputBindingDescription =
  {
    .binding = 0, .stride = sizeof(ImDrawVert), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };
  const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[3] =
  {
    (VkVertexInputAttributeDescription){.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ImDrawVert, pos)},
    (VkVertexInputAttributeDescription){.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ImDrawVert, uv)},
    (VkVertexInputAttributeDescription){.location = 2, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(ImDrawVert, col)}
  };
  const VkPipelineVertexInputStateCreateInfo vertexInputInfo = atlrInitVertexInputStateInfo(1, &vertexInputBindingDescription, 3, vertexInputAttributeDescriptions);

  const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = atlrInitPipelineInputAssemblyStateInfo();
  const VkPipelineViewportStateCreateInfo viewportInfo           = atlrInitPipelineViewportStateInfo();
  
  VkPipelineRasterizationStateCreateInfo rasterizationInfo = atlrInitPipelineRasterizationStateInfo();
  rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

  const VkPipelineMultisampleStateCreateInfo multisampleInfo     = atlrInitPipelineMultisampleStateInfo(device->msaaSamples);
  
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo   = atlrInitPipelineDepthStencilStateInfo();
  depthStencilInfo.depthTestEnable = VK_FALSE;
  
  const VkPipelineColorBlendAttachmentState colorBlendAttachment = atlrInitPipelineColorBlendAttachmentStateAlpha();
  const VkPipelineColorBlendStateCreateInfo colorBlendInfo       = atlrInitPipelineColorBlendStateInfo(&colorBlendAttachment);
  const VkPipelineDynamicStateCreateInfo dynamicInfo             = atlrInitPipelineDynamicStateInfo();

  const VkPushConstantRange pushConstantRange =
  {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(Transform)
  };
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo = atlrInitPipelineLayoutInfo(1, &this->descriptorSetLayout.layout, 1, &pushConstantRange);

  if(!atlrInitGraphicsPipeline(&this->pipeline,
			       2, stageInfos, &vertexInputInfo, &inputAssemblyInfo, NULL, &viewportInfo, &rasterizationInfo, &multisampleInfo, &depthStencilInfo, &colorBlendInfo, &dynamicInfo, &pipelineLayoutInfo,
			       device, &swapchain->renderPass))
  {
    std::runtime_error("atlrInitGraphicsPipeline returned 0.");
    return;
  }

  atlrDeinitShaderModule(vertexModule, device);
  atlrDeinitShaderModule(fragmentModule, device);

  this->vertexBuffers.resize(frameCount);
  this->vertexCounts.resize(frameCount);
  this->indexBuffers.resize(frameCount);
  this->indexCounts.resize(frameCount);
}

void Atlr::ImguiContext::deinit()
{
  const AtlrDevice* device = this->device;

  for (AtlrU8 i = 0; i < this->vertexBuffers.size(); i++)
  {
    if (this->vertexCounts[i])
      atlrDeinitBuffer(&this->vertexBuffers[i]);
    if (this->indexCounts[i])
      atlrDeinitBuffer(&this->indexBuffers[i]);
  }
  atlrDeinitPipeline(&this->pipeline);
  atlrDeinitDescriptorPool(&this->descriptorPool);
  atlrDeinitDescriptorSetLayout(&this->descriptorSetLayout);
  vkDestroySampler(device->logical, this->fontSampler, device->instance->allocator);
  atlrDeinitImage(&this->fontImage);
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void Atlr::ImguiContext::bind(const VkCommandBuffer commandBuffer, const AtlrU8 currentFrame)
{
  ImGuiIO& io = ImGui::GetIO();
  
  const VkDescriptorSet* set = &this->descriptorSets[currentFrame];
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.layout, 0, 1, set, 0, NULL);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline.pipeline);

  GLFWwindow* window = (GLFWwindow*)this->device->instance->data;
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  io.DisplaySize = ImVec2((float)width, (float)height);
  
  ImGui::NewFrame();
}

void Atlr::ImguiContext::draw(const VkCommandBuffer commandBuffer, const AtlrU8 currentFrame)
{
  ImGui::Render();
  ImGuiIO& io = ImGui::GetIO();
  ImDrawData* drawData = ImGui::GetDrawData();

  const AtlrU64 verticesSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
  const AtlrU64 indicesSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);
  const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  
  if (!verticesSize || !indicesSize) return;

#ifdef ATLR_DEBUG
  const float color[4] = {0.2f, 0.9f, 0.9f, 1.0f};
  atlrBeginCommandLabel(commandBuffer, "Imgui draw", color, this->device->instance);
#endif
  
  AtlrU32* vertexCount = &this->vertexCounts[currentFrame];
  AtlrBuffer* vertexBuffer = &this->vertexBuffers[currentFrame];
  if (drawData->TotalVtxCount != *vertexCount)
  {
    if (*vertexCount)
    {
      atlrUnmapBuffer(vertexBuffer);
      atlrDeinitBuffer(vertexBuffer);
    }
    
    *vertexCount = drawData->TotalVtxCount;
      
    if (*vertexCount)
    {
      const VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      if (!atlrInitBuffer(vertexBuffer, verticesSize, usage, memoryProperties, this->device))
      {
	throw std::runtime_error("atlrInitBuffer returned 0.");
	return;
      }
       atlrMapBuffer(vertexBuffer, 0, verticesSize, 0);
    }
  }

  AtlrU32* indexCount = &this->indexCounts[currentFrame];
  AtlrBuffer* indexBuffer = &this->indexBuffers[currentFrame];
  if (drawData->TotalIdxCount != *indexCount)
  {
    if (*indexCount)
    {
      atlrUnmapBuffer(indexBuffer);
      atlrDeinitBuffer(indexBuffer);
    }
    
    *indexCount = drawData->TotalIdxCount;

    if (*indexCount)
    {
      const VkBufferUsageFlags usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      if (!atlrInitBuffer(indexBuffer, indicesSize, usage, memoryProperties, this->device))
      {
	throw std::runtime_error("atlrInitBuffer returned 0.");
	return;
      }
      atlrMapBuffer(indexBuffer, 0, indicesSize, 0);
    }
  }

  if (!drawData->CmdListsCount) return;
  ImDrawVert* vertices = (ImDrawVert*)vertexBuffer->data;
  ImDrawIdx* indices = (ImDrawIdx*)indexBuffer->data;
  for (AtlrU32 i = 0; i < drawData->CmdListsCount; i++)
  {
    const ImDrawList* cmdList = drawData->CmdLists[i];
    
    memcpy(vertices, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
    vertices += cmdList->VtxBuffer.Size;

    memcpy(indices, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
    indices += cmdList->IdxBuffer.Size;
  }
  // non-coherent memory must be flushed
  atlrFlushBuffer(vertexBuffer, 0, verticesSize);
  atlrFlushBuffer(indexBuffer, 0, indicesSize);
  
  this->transform.translate = (AtlrVec2){{-1.0f, -1.0f}};
  this->transform.scale     = (AtlrVec2){{2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y}};
  vkCmdPushConstants(commandBuffer, this->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(this->transform), &this->transform);

  atlrCommandSetViewport(commandBuffer, io.DisplaySize.x, io.DisplaySize.y);
  
  AtlrI32 vertexOffset = 0;
  AtlrI32 indexOffset = 0;
  const VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->buffer, &offset);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT16);
  for (AtlrI32 i = 0; i < drawData->CmdListsCount; i++)
  {
    const ImDrawList* cmdList = drawData->CmdLists[i];
    for (AtlrI32 j = 0; j < cmdList->CmdBuffer.Size; j++)
    {
      const ImDrawCmd* cmd = &cmdList->CmdBuffer[j];
      const VkOffset2D offset = {.x = std::max((AtlrI32)cmd->ClipRect.x, 0), .y = std::max((AtlrI32)cmd->ClipRect.y, 0)};
      const VkExtent2D extent = {.width = (AtlrU32)(cmd->ClipRect.z - cmd->ClipRect.x), .height = (AtlrU32)(cmd->ClipRect.w - cmd->ClipRect.y)};
      atlrCommandSetScissor(commandBuffer, &offset, &extent);

      vkCmdDrawIndexed(commandBuffer, cmd->ElemCount, 1, indexOffset, vertexOffset, 0);
      indexOffset += cmd->ElemCount;
    }

    vertexOffset += cmdList->VtxBuffer.Size;
  }

#ifdef ATLR_DEBUG
  atlrEndCommandLabel(commandBuffer, this->device->instance);
#endif
}
