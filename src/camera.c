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

#include "camera.h"
#include <stdio.h>

#ifdef ATLR_BUILD_HOST_GLFW
AtlrU8 atlrInitPerspectiveCameraHostGLFW(AtlrPerspectiveCamera* restrict camera, const AtlrU8 frameCount, const float fov, const float nearPlane, const float farPlane,
					 const AtlrDevice* restrict device)
{
  const AtlrU64 size = sizeof(camera->uniformData);
  camera->device = device;
  camera->frameCount = frameCount;

  camera->uniformBuffers = malloc(frameCount * sizeof(AtlrBuffer));
  const VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  const VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  for (AtlrU8 i = 0; i < frameCount; i++)
  {
    if (!atlrInitBuffer(camera->uniformBuffers + i, size, usage, memoryProperties, device))
    {
      ATLR_ERROR_MSG("atlrInitBuffer returned 0.");
      return 0;
    }
    if (!atlrMapBuffer(camera->uniformBuffers + i, 0, size, 0))
    {
      ATLR_ERROR_MSG("atlrMapBuffer returned 0.");
      return 0;
    }
#ifdef ATLR_DEBUG
    char bufferString[64];
    sprintf(bufferString, "Camera Uniform Buffer ; Frame %d", i);
    atlrSetBufferName(camera->uniformBuffers + i, bufferString);
#endif
  }

  const VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  const VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    const VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = atlrInitDescriptorSetLayoutBinding(0, type, stages);
  if (!atlrInitDescriptorSetLayout(&camera->descriptorSetLayout, 1, &descriptorSetLayoutBinding, device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorSetLayout returned 0.");
    return 0;
  }

  const VkDescriptorPoolSize poolSize = atlrInitDescriptorPoolSize(type, frameCount);
  if (!atlrInitDescriptorPool(&camera->descriptorPool, frameCount, 1, &poolSize, device))
  {
    ATLR_ERROR_MSG("atlrInitDescriptorPool returned 0.");
    return 0;
  }

  camera->descriptorSets = malloc(frameCount * sizeof(VkDescriptorSet));
  VkDescriptorSetLayout* setLayouts = malloc(frameCount * sizeof(VkDescriptorSetLayout));
  for (AtlrU8 i = 0; i < frameCount; i++) setLayouts[i] = camera->descriptorSetLayout.layout;
  if (!atlrAllocDescriptorSets(&camera->descriptorPool, frameCount, setLayouts, camera->descriptorSets))
  {
    ATLR_ERROR_MSG("atlrAllocDescriptorSets returned 0.");
    free(setLayouts);
    return 0;
  }
  free(setLayouts);

  VkDescriptorBufferInfo* bufferInfos = malloc(frameCount * sizeof(VkDescriptorBufferInfo));
  VkWriteDescriptorSet* descriptorWrites = malloc(frameCount * sizeof(VkWriteDescriptorSet));
  for (AtlrU8 i = 0; i < frameCount; i++)
  {
    bufferInfos[i] = atlrInitDescriptorBufferInfo(camera->uniformBuffers + i, sizeof(camera->uniformData));
    descriptorWrites[i] = atlrWriteBufferDescriptorSet(camera->descriptorSets[i], 0, type, bufferInfos + i);
  }
  vkUpdateDescriptorSets(device->logical, frameCount, descriptorWrites, 0, NULL);
  free(bufferInfos);
  free(descriptorWrites);

  camera->fov = fov;
  camera->nearPlane = nearPlane;
  camera->farPlane = farPlane;

  camera->uniformData.eyePos = (AtlrVec4){{0.0f, 0.0f, 0.0f, 0.0f}};
  camera->uniformData.view = (AtlrMat4){{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};

  GLFWwindow* window = device->instance->data;
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  camera->uniformData.perspective = atlrPerspectiveProjection(fov, (float)height / width, nearPlane, farPlane);

  return 1;
}

void atlrDeinitPerspectiveCameraHostGLFW(const AtlrPerspectiveCamera* restrict camera)
{
  atlrDeinitDescriptorPool(&camera->descriptorPool);
  free(camera->descriptorSets);
  atlrDeinitDescriptorSetLayout(&camera->descriptorSetLayout);

  for (AtlrU8 i = 0; i < camera->frameCount; i++)
  {
    AtlrBuffer* uniformBuffer = camera->uniformBuffers + i;
    atlrUnmapBuffer(uniformBuffer);
    atlrDeinitBuffer(uniformBuffer);
  }
  free(camera->uniformBuffers);
}

void atlrUpdatePerspectiveCameraHostGLFW(AtlrPerspectiveCamera* restrict camera, const AtlrU8 currentFrame)
{
  GLFWwindow* window = camera->device->instance->data;
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  camera->uniformData.perspective = atlrPerspectiveProjection(camera->fov, (float)height / width, camera->nearPlane, camera->farPlane);
  memcpy(camera->uniformBuffers[currentFrame].data, &camera->uniformData, sizeof(camera->uniformData));
}

void atlrPerspectiveCameraLookAtHostGLFW(AtlrPerspectiveCamera* restrict camera, const AtlrVec3* restrict eyePos, const AtlrVec3* restrict targetPos, const AtlrVec3* restrict worldUpDir)
{
  camera->uniformData.eyePos = (AtlrVec4){{eyePos->x, eyePos->y, eyePos->z, 0.0f}};
  camera->uniformData.view = atlrLookAt(eyePos, targetPos, worldUpDir);
}
#endif
