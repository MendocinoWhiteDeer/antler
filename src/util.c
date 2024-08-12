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
