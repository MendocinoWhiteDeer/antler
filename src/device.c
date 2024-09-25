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

// if graphics is supported Vulkan demands at least one family supporting both graphics and compute 
// find queue families supporting graphics/compute and present; prioritize any family with both
static void initQueueFamilyIndices(AtlrQueueFamilyIndices* restrict indices, const AtlrInstance* restrict instance, const VkPhysicalDevice physical)
{
  AtlrU32 count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, NULL);
  VkQueueFamilyProperties* properties = malloc(count * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, properties);

  for (AtlrU8 i = 0; i < count; i++)
  {
    const VkQueueFlags queueFlags = properties[i].queueFlags;
    const VkBool32 graphicsComputeSupport = (queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFlags & VK_QUEUE_COMPUTE_BIT);
    VkBool32 presentSupport = 0;
    if (instance->surface != VK_NULL_HANDLE)
      vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, instance->surface, &presentSupport);

    if (graphicsComputeSupport && presentSupport)
    {
      indices->isGraphicsCompute = 1;
      indices->isPresent = 1;
      indices->graphicsComputeIndex = i;
      indices->presentIndex = i;
      break;
    }
    else if (!indices->isGraphicsCompute && graphicsComputeSupport)
    {
      indices->isGraphicsCompute = 1;
      indices->graphicsComputeIndex = i;
    }
    else if (!indices->isPresent && presentSupport)
    {
      indices->isPresent = 1;
      indices->presentIndex = i;
    }
  }

  free(properties);
}

AtlrU8 atlrInitSwapchainSupportDetails(AtlrSwapchainSupportDetails* restrict support, const AtlrInstance* restrict instance, const VkPhysicalDevice physical)
{
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, instance->surface, &support->capabilities) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkGetPhysicalDeviceSurfaceCapabilitiesKHR did not return VK_SUCCESS.");
    return 0;
  }

  if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical, instance->surface, &support->formatCount, NULL) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkGetPhysicalDeviceSurfaceFormatsKHR (first call) did not return VK_SUCCESS.");
    return 0;
  }
  if (support->formatCount)
  {
    support->formats = malloc(support->formatCount * sizeof(VkSurfaceFormatKHR));
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical, instance->surface, &support->formatCount, support->formats) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkGetPhysicalDeviceSurfaceFormatsKHR (second call) did not return VK_SUCCESS.");
      return 0;
    }
  }

  if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical, instance->surface, &support->presentModeCount, NULL) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkGetPhysicalDeviceSurfacePresentModesKHR (first call) did not return VK_SUCCESS.");
    return 0;
  }
  if (support->presentModeCount)
  {
    support->presentModes = malloc(support->presentModeCount * sizeof(VkPresentModeKHR));
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical, instance->surface, &support->presentModeCount,support->presentModes) != VK_SUCCESS)
    {
      ATLR_ERROR_MSG("vkGetPhysicalDeviceSurfacePresentModesKHR (second call) did not return VK_SUCCESS.");
      return 0;
    }
  }

  return 1;
}

void atlrDeinitSwapchainSupportDetails(AtlrSwapchainSupportDetails* restrict support)
{
  free(support->formats);
  free(support->presentModes);
}

#if defined(ATLR_BUILD_HOST_HEADLESS) || defined(ATLR_BUILD_HOST_GLFW)
static const char* deviceCriterionNames[ATLR_DEVICE_CRITERION_TOT] =
{
  "AT LEAST VULKAN VERSION 1.1",
  "AT LEAST VULKAN VERSION 1.2",
  "AT LEAST VULKAN VERSION 1.3",
  
  "OTHER PHYSICAL DEVICE",
  "INTEGRATED GPU PHYSICAL DEVICE",
  "DISCRETE GPU PHYSICAL DEVICE",
  "VIRTUAL GPU PHYSICAL DEVICE",
  "CPU PHYSICAL DEVICE",

  "QUEUE FAMILY GRAPHICS SUPPORT",
  "QUEUE FAMILY PRESENT SUPPORT",
  "QUEUE FAMILY COMPUTE SUPPORT",

  "SWAPCHAIN SUPPORT",

  "GEOMETRY SHADER"
};

static AtlrU8 arePhysicalDeviceExtensionsAvailable(const VkPhysicalDevice physical, const char** restrict extensions, AtlrU32 extensionCount)
{
  AtlrU32 availableExtensionCount = 0;
  if (vkEnumerateDeviceExtensionProperties(physical, NULL, &availableExtensionCount, NULL) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkEnumerateDeviceExtensionProperties (first call) did not return VK_SUCCESS.");
    return 0;
  }
  VkExtensionProperties* availableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
  if (vkEnumerateDeviceExtensionProperties(physical, NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkEnumerateDeviceExtensionProperties (second call) did not return VK_SUCCESS.");
    free(availableExtensions);
    return 0;
  }
   
  AtlrU8 extensionsFound = 1;
  for (AtlrU32 i = 0; i < extensionCount; i++)
  {
    AtlrU8 found = 0;
    for (AtlrU32 j = 0; j < availableExtensionCount; j++)
      if (!strcmp(extensions[i], availableExtensions[j].extensionName))
	{
	  found = 1;
	  atlrLog(ATLR_LOG_DEBUG, "Vulkan device extension \"%s\" is available.", extensions[i]);
	  break;
	}
    if (!found)
      {
	atlrLog(ATLR_LOG_DEBUG, "Vulkan device extension \"%s\" is unavailable.", extensions[i]);
	extensionsFound = 0;
      }
  }

  free(availableExtensions);
  return extensionsFound;
}

void atlrInitDeviceCriteria(AtlrDeviceCriterion* restrict criteria)
{
  for (AtlrI32 i = 0; i < ATLR_DEVICE_CRITERION_TOT; i++)
  {
    AtlrDeviceCriterion* criterion = criteria + i;
    criterion->method = ATLR_DEVICE_CRITERION_METHOD_POINT_SHIFT;
    criterion->pointShift = 0;
  }
}

AtlrU8 atlrSetDeviceCriterion(AtlrDeviceCriterion* restrict criteria, AtlrDeviceCriterionType type, AtlrDeviceCriterionMethod method, AtlrI32 pointShift)
{
  if (type >= ATLR_DEVICE_CRITERION_TOT) return 0;
  AtlrDeviceCriterion* criterion = criteria + type;
  criterion->method = method;
  criterion->pointShift = pointShift;
  return 1;
}

AtlrU8 atlrInitDeviceHost(AtlrDevice* restrict device, const AtlrInstance* restrict instance, const AtlrDeviceCriterion* restrict criteria)
{
  {
    AtlrDevice temp = {};
    *device = temp;
  }
 
#ifdef ATLR_BUILD_HOST_HEADLESS
  atlrLog(ATLR_LOG_INFO, "Initializing Antler device in host headless mode ...");
#elif ATLR_BUILD_HOST_GLFW
  atlrLog(ATLR_LOG_INFO, "Initializing Antler device in host GLFW mode ...");
#endif
  device->instance = instance;
  
  AtlrU32 physicalDeviceCount = 0;
  if (vkEnumeratePhysicalDevices(instance->instance, &physicalDeviceCount, NULL) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkEnumeratePhysicalDevices (first call) did not return VK_SUCCESS.");
    return 0;
  }
  if (!physicalDeviceCount)
  {
    ATLR_ERROR_MSG("No physical devices with Vulkan support.");
    return 0;
  }
  VkPhysicalDevice* physicalDevices = malloc(physicalDeviceCount * sizeof(VkPhysicalDevice));
  if (vkEnumeratePhysicalDevices(instance->instance, &physicalDeviceCount, physicalDevices) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkEnumeratePhysicalDevices (second call) did not return VK_SUCCESS.");
    free(physicalDevices);
    return 0;
  }

  // Check criteria and bestow each physical device a grade, the device with the best grade is selected
  AtlrI32 bestGrade = 0;
  AtlrU8 foundPhysicalDevice = 0;
  static const char* swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  atlrLog(ATLR_LOG_DEBUG,
	     "Physical devices may be numerically graded based on different user-provided criteria.\n"
	     "The physical device with the best grade is chosen.\n"
	     "A criterion method determines whether a criterion is (a) required, (b) forbidden, or (c) point-shifted.\n"
	     "Violating a criterion that is (a) or (b) locks into a failing grade. Physical devices that fail will not be ranked.\n"
	     "Criterion with (c) apply a (positive or negative) point shift to the grade when satisfied, and zero change to the grade otherwise.");
  atlrLog(ATLR_LOG_DEBUG, "Grading physical devices based on device criteria ...");
  for (AtlrU32 i = 0; i < physicalDeviceCount; i++)
  { 
    const VkPhysicalDevice physical = physicalDevices[i];
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceProperties(physical, &properties);
    vkGetPhysicalDeviceFeatures(physical, &features);
    vkGetPhysicalDeviceMemoryProperties(physical, &memoryProperties);

    atlrLog(ATLR_LOG_DEBUG, "Grading physical device \"%s\" ...", properties.deviceName);
    
    const AtlrU32 version = properties.apiVersion;
    const AtlrU32 versionMajor = VK_VERSION_MAJOR(version);
    const AtlrU32 versionMinor = VK_VERSION_MINOR(version);

    AtlrQueueFamilyIndices queueFamilyIndices;
    initQueueFamilyIndices(&queueFamilyIndices, instance, physical);

    AtlrSwapchainSupportDetails swapchainSupportDetails = {};
    const AtlrU8 hasSwapchainSupport =
      (instance->surface != VK_NULL_HANDLE)
      && arePhysicalDeviceExtensionsAvailable(physical, &swapchainExtension, 1)
      && atlrInitSwapchainSupportDetails(&swapchainSupportDetails, instance, physical);

    AtlrI32 grade = 0;
    AtlrU8 isFailureLocked = 0;

    AtlrU8 criterionValues[ATLR_DEVICE_CRITERION_TOT];
    for (AtlrI32 j = 0; j < ATLR_DEVICE_CRITERION_TOT; j++)
    {
      switch(j)
      {
        case ATLR_DEVICE_CRITERION_VULKAN_VERSION_AT_LEAST_1_1:
	  criterionValues[j] = (versionMajor > 1) || ((versionMajor == 1) && (versionMinor >= 1));
	  break;
        case ATLR_DEVICE_CRITERION_VULKAN_VERSION_AT_LEAST_1_2:
	  criterionValues[j] = (versionMajor > 1) || ((versionMajor == 1) && (versionMinor >= 2));
	  break;
        case ATLR_DEVICE_CRITERION_VULKAN_VERSION_AT_LEAST_1_3:
	  criterionValues[j] = (versionMajor > 1) || ((versionMajor == 1) && (versionMinor >= 3));
	  break;
	
        case ATLR_DEVICE_CRITERION_OTHER_PHYSICAL_DEVICE:
	  criterionValues[j] = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER);
	  break;
        case ATLR_DEVICE_CRITERION_INTEGRATED_GPU_PHYSICAL_DEVICE:
	  criterionValues[j] = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
	  break;
        case ATLR_DEVICE_CRITERION_DISCRETE_GPU_PHYSICAL_DEVICE:
	  criterionValues[j] = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
	  break;
        case ATLR_DEVICE_CRITERION_VIRTUAL_GPU_PHYSICAL_DEVICE:
	  criterionValues[j] = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU);
	  break;
        case ATLR_DEVICE_CRITERION_CPU_PHYSICAL_DEVICE:
	  criterionValues[j] = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU);
	  break;

        case ATLR_DEVICE_CRITERION_QUEUE_FAMILY_GRAPHICS_SUPPORT:
	  criterionValues[j] = queueFamilyIndices.isGraphicsCompute;
	  break;
        case ATLR_DEVICE_CRITERION_QUEUE_FAMILY_PRESENT_SUPPORT:
	  criterionValues[j] = queueFamilyIndices.isPresent;
	  break;
        case ATLR_DEVICE_CRITERION_QUEUE_FAMILY_COMPUTE_SUPPORT:
	  criterionValues[j] = queueFamilyIndices.isGraphicsCompute;
	  break;

        case ATLR_DEVICE_CRITERION_SWAPCHAIN_SUPPORT:
	  criterionValues[j] = hasSwapchainSupport;
	  break;

        case ATLR_DEVICE_CRITERION_GEOMETRY_SHADER:
	  criterionValues[j] = features.geometryShader;
	  break;
      }
    }

    for (AtlrI32 j = 0; j < ATLR_DEVICE_CRITERION_TOT; j++)
    {
      const AtlrDeviceCriterion* criterion = criteria + j;
      const AtlrU8 criterionValue = criterionValues[j];
      const char* criterionName = deviceCriterionNames[j];
      switch (criterion->method)
      {
        case ATLR_DEVICE_CRITERION_METHOD_POINT_SHIFT:
	{
	  if (!criterion->pointShift) break;
	  const char* met = criterionValue ? "is met" : "is not met";
	  
	  if (isFailureLocked)
	  {
	    atlrLog(ATLR_LOG_DEBUG, "Criterion (type: \"%s\", method: point-shift by %d) %s. "
		       "The physical device is locked into a failing grade regardless.",
		       criterionName, criterion->pointShift,  met);
	    break;
	  }
	  
	  grade += criterionValue ? criterion->pointShift : 0;
	  atlrLog(ATLR_LOG_DEBUG, "Criterion (type: \"%s\", method: point-shift by %d) %s. "
		     "The current grade is %d.",
		     criterionName, criterion->pointShift, met, grade);
	  break;
	}

        case ATLR_DEVICE_CRITERION_METHOD_REQUIRED:
	{
	  const char* met = criterionValue ? "is met" : "is not met";
	  
	  if (isFailureLocked)
	  {
	    atlrLog(ATLR_LOG_DEBUG, "Criterion (type: \"%s\", method: required) %s. "
		       "The physical device is locked into a failing grade regardless.",
		       criterionName, met);
	    break;
	  }

	  if (criterionValue)
	    atlrLog(ATLR_LOG_DEBUG, "Criterion (type: \"%s\", method: required) %s. "
		       "The current grade is %d.",
		       criterionName, met, grade);
	  else
	  {
	    atlrLog(ATLR_LOG_DEBUG, "Criterion (type: \"%s\", method: required) %s. "
		       "The physical device is now locked into a failing grade.",
		       criterionName, met);
	    isFailureLocked = 1;
	  }
	  break;
	}

        case ATLR_DEVICE_CRITERION_METHOD_FORBIDDEN:
	{
	  const char* met = criterionValue ? "is not met" : "is met";
	  
	  if (isFailureLocked)
	  {
	    atlrLog(ATLR_LOG_DEBUG, "Criterion (type: \"%s\", method: forbidden) %s. "
		       "The physical device is locked into a failing grade regardless.",
		       criterionName, met);
	    break;
	  }

	  if (criterionValue)
	  {
	    atlrLog(ATLR_LOG_DEBUG, "Criterion (type: \"%s\", method: forbidden) %s. "
		       "The physical device is now locked into a failing grade.",
		       criterionName, met);
	    isFailureLocked = 1;
	  }
	  else
	    atlrLog(ATLR_LOG_DEBUG, "Criterion (type: \"%s\", method: forbidden) %s. "
		       "The current grade is %d.",
		       criterionName, met, grade);
	  break;
	}
      }
    }

    if (isFailureLocked)
      atlrLog(ATLR_LOG_INFO, "The physical device \"%s\" received a failing grade.", properties.deviceName);
    else
      atlrLog(ATLR_LOG_INFO, "The physical device \"%s\" received a grade of %d.", properties.deviceName, grade);
    if (!isFailureLocked && ((grade > bestGrade) || !foundPhysicalDevice))
    {
      bestGrade = grade;
      foundPhysicalDevice = 1;
      device->physical = physical;
      device->queueFamilyIndices = queueFamilyIndices;
      device->hasSwapchainSupport = hasSwapchainSupport;
    }
    atlrDeinitSwapchainSupportDetails(&swapchainSupportDetails);
  }
  free(physicalDevices);

  if (!foundPhysicalDevice)
  {
    ATLR_ERROR_MSG("An appropriate physical device was never found.");
    return 0;
  }
  
  // set device info
  VkPhysicalDeviceFeatures deviceFeatures = {};
  {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device->physical, &properties);
    vkGetPhysicalDeviceFeatures(device->physical, &features);
    
    atlrLog(ATLR_LOG_INFO, "With the highest grade of %d, the physical device \"%s\" was selected.",
	       bestGrade, properties.deviceName);
    
    AtlrDeviceCriterion geometryShaderCriterion = criteria[ATLR_DEVICE_CRITERION_GEOMETRY_SHADER];
    switch (geometryShaderCriterion.method)
    {
      case ATLR_DEVICE_CRITERION_METHOD_POINT_SHIFT:
	deviceFeatures.geometryShader = (geometryShaderCriterion.pointShift >= 0 && features.geometryShader) ? VK_TRUE : VK_FALSE;
	break;
      case ATLR_DEVICE_CRITERION_METHOD_REQUIRED:
	deviceFeatures.geometryShader = VK_TRUE;
	break;
      case ATLR_DEVICE_CRITERION_METHOD_FORBIDDEN:
	deviceFeatures.geometryShader = VK_FALSE;
	break;
    }
    
    VkSampleCountFlags countFlags = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
    if (countFlags & VK_SAMPLE_COUNT_4_BIT)      device->msaaSamples = VK_SAMPLE_COUNT_4_BIT;
    else if (countFlags & VK_SAMPLE_COUNT_2_BIT) device->msaaSamples = VK_SAMPLE_COUNT_2_BIT;
    else                                         device->msaaSamples = VK_SAMPLE_COUNT_1_BIT;
  }

  AtlrU32 uniqueQueueFamilyIndices[2];
  const AtlrQueueFamilyIndices* queueFamilyIndices = &device->queueFamilyIndices; 
  AtlrU8 uniqueQueueFamilyIndicesCount = 0;
  if (queueFamilyIndices->isGraphicsCompute)
  {
    uniqueQueueFamilyIndices[uniqueQueueFamilyIndicesCount] = queueFamilyIndices->graphicsComputeIndex;
    uniqueQueueFamilyIndicesCount++;
  }
  if (queueFamilyIndices->isPresent &&
      (!queueFamilyIndices->isGraphicsCompute || (queueFamilyIndices->graphicsComputeIndex != queueFamilyIndices->presentIndex)))
  {
    uniqueQueueFamilyIndices[uniqueQueueFamilyIndicesCount] = queueFamilyIndices->presentIndex;
    uniqueQueueFamilyIndicesCount++;
  }
  VkDeviceQueueCreateInfo queueInfos[2];
  const float priority = 1.0f;
  for (AtlrU32 i = 0; i < uniqueQueueFamilyIndicesCount; i++)
    queueInfos[i] = (VkDeviceQueueCreateInfo)
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex = uniqueQueueFamilyIndices[i],
      .queueCount = 1,
      .pQueuePriorities = &priority
    };

  // create logical device; enabledLayerCount and ppEnabledLayerNames are deprecated fields
  VkDeviceCreateInfo deviceInfo =
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .queueCreateInfoCount = uniqueQueueFamilyIndicesCount,
    .pQueueCreateInfos = queueInfos,
    .pEnabledFeatures = &deviceFeatures
  };
  if (device->hasSwapchainSupport)
  {
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = &swapchainExtension;
  }
  else
  {
    deviceInfo.enabledExtensionCount = 0;
    deviceInfo.ppEnabledExtensionNames = NULL;
  }
  if (vkCreateDevice(device->physical, &deviceInfo, instance->allocator, &device->logical) != VK_SUCCESS)
  {
    ATLR_ERROR_MSG("vkCreateDevice did return VK_SUCCESS.");
    return 0;
  }

  if (queueFamilyIndices->isGraphicsCompute)
    vkGetDeviceQueue(device->logical, queueFamilyIndices->graphicsComputeIndex, 0, &device->graphicsComputeQueue);
  else
    device->graphicsComputeQueue = VK_NULL_HANDLE;
  if (queueFamilyIndices->isPresent)
    vkGetDeviceQueue(device->logical, queueFamilyIndices->presentIndex, 0, &device->presentQueue);
  else
    device->presentQueue = VK_NULL_HANDLE;

  atlrLog(ATLR_LOG_INFO, "Done initializing antler device.");
  return 1;
}

void atlrDeinitDeviceHost(AtlrDevice* device)
{
#ifdef ATLR_BUILD_HOST_HEADLESS
  atlrLog(ATLR_LOG_INFO, "Deinitializing Antler device in host headless mode ...");
#elif ATLR_BUILD_HOST_GLFW
  atlrLog(ATLR_LOG_INFO, "Deinitializing Antler device in host GLFW mode ...");
#endif

  vkDestroyDevice(device->logical, device->instance->allocator);

  atlrLog(ATLR_LOG_INFO, "Done deinitializing antler device.");
}
#endif
