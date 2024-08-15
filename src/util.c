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
#include <glslang/Public/resource_limits_c.h>

float atlrClampFloat(const float x, const float min, const float max)
{
  if (x < min) return min;
  else if (x > max) return max;
  else return x;
}

void* atlrAlignedMalloc(const AtlrU64 size, const AtlrU64 alignment)
{
#if defined(__MINGW32__)
  return _aligned_malloc(size, alignment);
#elif defined (__linux__)
  void* data = NULL;
  int result = posix_memalign(&data, alignment, size);
  if (result != 0) return NULL;
  return data;
#endif
}

void atlrAlignedFree(void* data)
{
#if defined(__MINGW32__)
  _aligned_free(data);
#elif defined (__linux__)
  free(data);
#endif
}

AtlrU8 atlrAlign(AtlrU64* aligned, const AtlrU64 offset, const AtlrU64 alignment)
{
  if (!alignment)
  {
    *aligned = offset;
    return 1;
  }
  
  AtlrU8 isPowerofTwo = alignment & (alignment - 1);
  if (!isPowerofTwo)
  {
    ATLR_ERROR_MSG("align must be zero or a power of two.");
    return 0;
  }
  
  *aligned = (offset + (alignment - 1)) & ~(alignment - 1); 
  return 1;
}

AtlrU8 atlrGetVulkanMemoryTypeIndex(AtlrU32* restrict index, const VkPhysicalDevice physical, const AtlrU32 typeFilter, const VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physical, &memoryProperties);
  
  for (AtlrU32 i = 0; i < memoryProperties.memoryTypeCount; i++)
   {
     if ((typeFilter & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
     {
       *index = i;
       return 1;
      }
   }

  ATLR_ERROR_MSG("No suitable buffer memory type.");
  return 0;
}

AtlrU8 atlrInitSpirVBinary(AtlrSpirVBinary* restrict bin, glslang_stage_t stage, const char* restrict glsl, const char* restrict name)
{
  const glslang_input_t input =
  {
    .language = GLSLANG_SOURCE_GLSL,
    .stage = stage,
    .client = GLSLANG_CLIENT_VULKAN,
    .client_version = GLSLANG_TARGET_VULKAN_1_3,
    .target_language = GLSLANG_TARGET_SPV,
    .target_language_version = GLSLANG_TARGET_SPV_1_6,
    .code = glsl,
    .default_version = 100,
    .default_profile = GLSLANG_NO_PROFILE,
    .force_default_version_and_profile = false,
    .forward_compatible = false,
    .messages = GLSLANG_MSG_DEFAULT_BIT,
    .resource = glslang_default_resource(),
    .callbacks = {},
    .callbacks_ctx = NULL
  };

  glslang_shader_t* shader = glslang_shader_create(&input);

  // preprocess
  if (!glslang_shader_preprocess(shader, &input))
  {
    ATLR_ERROR_MSG("glslang_shader_preprocess returned 0.");
    atlrLog(ATLR_LOG_DEBUG, "%s\n", glslang_shader_get_info_log(shader));
    atlrLog(ATLR_LOG_DEBUG, "%s\n", glslang_shader_get_info_debug_log(shader));
    atlrLog(ATLR_LOG_DEBUG, "%s\n", input.code);
    glslang_shader_delete(shader);
    return 0;
  }

  // parse
  if (!glslang_shader_parse(shader, &input))
  {
    ATLR_ERROR_MSG("glslang_shader_parse returned 0.");
    atlrLog(ATLR_LOG_DEBUG, "%s\n", glslang_shader_get_info_log(shader));
    atlrLog(ATLR_LOG_DEBUG, "%s\n", glslang_shader_get_info_debug_log(shader));
    atlrLog(ATLR_LOG_DEBUG, "%s\n", glslang_shader_get_preprocessed_code(shader));
    glslang_shader_delete(shader);
    return 0;
  }

  glslang_program_t* program = glslang_program_create();
  glslang_program_add_shader(program, shader);

  // link
  if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
  {
    ATLR_ERROR_MSG("glslang_shader_parse returned 0.");
    atlrLog(ATLR_LOG_DEBUG, "%s\n", glslang_shader_get_info_log(shader));
    atlrLog(ATLR_LOG_DEBUG, "%s\n", glslang_shader_get_info_debug_log(shader));
    glslang_program_delete(program);
    glslang_shader_delete(shader);
  }

  // generate
  glslang_program_SPIRV_generate(program, stage);
  const char* msg = glslang_program_SPIRV_get_messages(program);
  if (msg) atlrLog(ATLR_LOG_INFO, "Name:%s %s\b", name, msg); 

  bin->codeSize = sizeof(AtlrU32) * glslang_program_SPIRV_get_size(program);
  bin->code = malloc(bin->codeSize);
  glslang_program_SPIRV_get(program, bin->code);

  glslang_program_delete(program);
  glslang_shader_delete(shader);

  return 1;
}

void atlrDeinitSpirVBinary(AtlrSpirVBinary* restrict bin)
{
  free(bin->code);
}
