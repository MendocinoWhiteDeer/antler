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

static const char* deviceCriterionNames[ATLR_DEVICE_CRITERION_TOT] =
{
  // These criteria correspond to the Vulkan physical device types
  "OTHER PHYSICAL DEVICE",
  "INTEGRATED GPU PHYSICAL DEVICE",
  "DISCRETE GPU PHYSICAL DEVICE",
  "VIRTUAL GPU PHYSICAL DEVICE",
  "CPU PHYSICAL DEVICE"
};

// find queue families supporting graphics and present; prioritize any family with both
static void initQueueFamilyIndices(AtlrQueueFamilyIndices* restrict indices, const AtlrInstance* restrict instance, const VkPhysicalDevice physical)
{
  AtlrU32 count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, NULL);
  VkQueueFamilyProperties* properties = malloc(count * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, properties);

  for (AtlrU8 i = 0; i < count; i++)
  {
    VkBool32 graphicsSupport = properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
    VkBool32 presentSupport = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, instance->surface, &presentSupport);

    if (graphicsSupport && presentSupport)
    {
      indices->isGraphics = 1;
      indices->isPresent = 1;
      indices->graphics_index = i;
      indices->present_index = i;
      break;
    }
    else if (!indices->isGraphics && graphicsSupport)
    {
      indices->isGraphics = 1;
      indices->graphics_index = i;
    }
    else if (!indices->isPresent && presentSupport)
    {
      indices->isPresent = 1;
      indices->present_index = i;
    }
  }

  free(properties);
}

static AtlrU8 arePhysicalDeviceExtensionsAvailable(const VkPhysicalDevice physical, const char** restrict extensions, AtlrU32 extensionCount)
{
  AtlrU32 availableExtensionCount = 0;
  if (vkEnumerateDeviceExtensionProperties(physical, NULL, &availableExtensionCount, NULL) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_ERROR, "vkEnumerateDeviceExtensionProperties (first call) did not return VK_SUCCESS.");
    return 0;
  }
  VkExtensionProperties* availableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
  if (vkEnumerateDeviceExtensionProperties(physical, NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_ERROR, "vkEnumerateDeviceExtensionProperties (second call) did not return VK_SUCCESS.");
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
	  atlrLogMsg(LOG_DEBUG, "Vulkan device extension \"%s\" is available.", extensions[i]);
	  break;
	}
    if (!found)
      {
	atlrLogMsg(LOG_DEBUG, "Vulkan device extension \"%s\" is unavailable.", extensions[i]);
	extensionsFound = 0;
      }
  }

  free(availableExtensions);
  return extensionsFound;
}

static AtlrU8 initSwapchainSupportDetails(AtlrSwapchainSupportDetails* restrict support, const AtlrInstance* instance, const VkPhysicalDevice physical)
{
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, instance->surface, &support->capabilities) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_ERROR, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR did not return VK_SUCCESS.");
    return 0;
  }

  support->formatCount = 0;
  support->formats = NULL;
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical, instance->surface, &support->formatCount, NULL) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_ERROR, "vkGetPhysicalDeviceSurfaceFormatsKHR (first call) did not return VK_SUCCESS.");
    return 0;
  }
  if (support->formatCount)
  {
    support->formats = malloc(support->formatCount * sizeof(VkSurfaceFormatKHR));
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical, instance->surface, &support->formatCount, support->formats) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_ERROR, "vkGetPhysicalDeviceSurfaceFormatsKHR (second call) did not return VK_SUCCESS.");
      return 0;
    }
  }

  support->presentModeCount = 0;
  support->presentModes = NULL;
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical, instance->surface, &support->presentModeCount, NULL) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_ERROR, "vkGetPhysicalDeviceSurfacePresentModesKHR (first call) did not return VK_SUCCESS.");
    return 0;
  }
  if (support->presentModeCount)
  {
    support->presentModes = malloc(support->presentModeCount * sizeof(VkPresentModeKHR));
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical, instance->surface, &support->presentModeCount,support->presentModes) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_ERROR, "vkGetPhysicalDeviceSurfacePresentModesKHR (second call) did not return VK_SUCCESS.");
      return 0;
    }
  }

  return 1;
}

static void deinitSwapchainSupportDetails(AtlrSwapchainSupportDetails* restrict support)
{
  free(support->formats);
  free(support->presentModes);
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

AtlrU8 atlrInitDevice(AtlrDevice* restrict device, const AtlrInstance* restrict instance, AtlrDeviceCriterion* restrict criteria)
{
  atlrLogMsg(LOG_INFO, "Initializing antler device ...");
  
  AtlrU32 physicalDeviceCount = 0;
  if (vkEnumeratePhysicalDevices(instance->instance, &physicalDeviceCount, NULL) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_FATAL, "vkEnumeratePhysicalDevices (first call) did not return VK_SUCCESS.");
    return 0;
  }
  if (!physicalDeviceCount)
  {
    atlrLogMsg(LOG_FATAL, "No physical devices with Vulkan support.");
    return 0;
  }
  VkPhysicalDevice* physicalDevices = malloc(physicalDeviceCount * sizeof(VkPhysicalDevice));
  if (vkEnumeratePhysicalDevices(instance->instance, &physicalDeviceCount, physicalDevices) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_FATAL, "vkEnumeratePhysicalDevices (second call) did not return VK_SUCCESS.");
    free(physicalDevices);
    return 0;
  }

  // Check criteria and bestow each physical device a grade, the device with the best grade is selected
  AtlrU32 bestGrade = 0;
  AtlrU8 foundPhysicalDevice = 0;
  static const char* swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  atlrLogMsg(LOG_DEBUG,
	     "Physical devices may be numerically graded based on different user-provided criteria.\n"
	     "The physical device with the best grade is chosen.\n"
	     "A criterion method determines whether a criterion is (a) required, (b) forbidden, or (c) point-shifted.\n"
	     "Violating a criterion that is (a) or (b) locks into a failing grade. Physical devices that fail will not be ranked.\n"
	     "In addition to user-provided criteria, there are some base requirements that need to be checked.\n"
	     "Criterion with (c) apply a (positive or negative) point shift to the grade when satisfied, and zero change to the grade otherwise.");
  atlrLogMsg(LOG_DEBUG, "Grading physical devices based on device criteria  ...");
  for (AtlrU32 i = 0; i < physicalDeviceCount; i++)
  { 
    const VkPhysicalDevice physical = physicalDevices[i];
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceProperties(physical, &properties);
    vkGetPhysicalDeviceFeatures(physical, &features);
    vkGetPhysicalDeviceMemoryProperties(physical, &memoryProperties);

    atlrLogMsg(LOG_DEBUG, "Grading physical device \"%s\" ...", properties.deviceName);
    AtlrI32 grade = 0;
    AtlrU8 isFailureLocked = 0;

    atlrLogMsg(LOG_DEBUG, "First checking basic requirements ...");

    // Vulkan version requirements
    {
      const AtlrU32 version = properties.apiVersion;
      const AtlrU32 versionMajor = VK_VERSION_MAJOR(version);
      const AtlrU32 versionMinor = VK_VERSION_MINOR(version);
      if (versionMajor == 1 && versionMinor < 3)
      {
	isFailureLocked = 1;
	atlrLogMsg(LOG_DEBUG, "The physical device does not support Vulkan 1.3 or later.");
      }
      else atlrLogMsg(LOG_DEBUG, "The physical device supports Vulkan 1.3 or later.");
    }

    // queue requirements
    AtlrQueueFamilyIndices queueFamilyIndices;
    initQueueFamilyIndices(&queueFamilyIndices, instance, physical);
    if (!queueFamilyIndices.isGraphics || !queueFamilyIndices.isPresent)
    {
      isFailureLocked = 1;
      atlrLogMsg(LOG_DEBUG, "Queue requirements are not met.");
    }
    else atlrLogMsg(LOG_DEBUG, "Queue requirements are met.");

    // swapchain requirements
    AtlrSwapchainSupportDetails swapchainSupportDetails;
    if (!arePhysicalDeviceExtensionsAvailable(physical, &swapchainExtension, 1)
	|| !initSwapchainSupportDetails(&swapchainSupportDetails, instance, physical))
    {
      isFailureLocked = 1;
      atlrLogMsg(LOG_DEBUG, "Swapchain requirements are not met.");
    }
    else atlrLogMsg(LOG_DEBUG, "Swapchain requirements are met.");

    if (isFailureLocked)
      atlrLogMsg(LOG_DEBUG, "The device failed to meet basic requirements. "
		 "It is now locked into receiving a failing grade.");

    // user-specified criteria
    atlrLogMsg(LOG_DEBUG, "Now checking user-specified criteria ...");
    AtlrU8 criterionValues[ATLR_DEVICE_CRITERION_TOT];
    for (AtlrI32 j = 0; j < ATLR_DEVICE_CRITERION_TOT; j++)
    {
      switch(j)
      {
	// Vulkan physical device type criteria
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
	    atlrLogMsg(LOG_DEBUG, "Criterion (type: \"%s\", method: point-shift by %d) %s. "
		       "The physical device is locked into a failing grade regardless.",
		       criterionName, criterion->pointShift,  met);
	    break;
	  }
	  
	  grade += criterionValue ? criterion->pointShift : 0;
	  atlrLogMsg(LOG_DEBUG, "Criterion (type: \"%s\", method: point-shift by %d) %s. "
		     "The current grade is %d.",
		     criterionName, criterion->pointShift, met, grade);
	  break;
	}

        case ATLR_DEVICE_CRITERION_METHOD_REQUIRED:
	{
	  const char* met = criterionValue ? "is met" : "is not met";
	  
	  if (isFailureLocked)
	  {
	    atlrLogMsg(LOG_DEBUG, "Criterion (type: \"%s\", method: required) %s. "
		       "The physical device is locked into a failing grade regardless.",
		       criterionName, met);
	    break;
	  }

	  if (criterionValue)
	    atlrLogMsg(LOG_DEBUG, "Criterion (type: \"%s\", method: required) %s. "
		       "The current grade is %d.",
		       criterionName, met, grade);
	  else
	  {
	    atlrLogMsg(LOG_DEBUG, "Criterion (type: \"%s\", method: required) %s. "
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
	    atlrLogMsg(LOG_DEBUG, "Criterion (type: \"%s\", method: forbidden) %s. "
		       "The physical device is locked into a failing grade regardless.",
		       criterionName, met);
	    break;
	  }

	  if (criterionValue)
	  {
	    atlrLogMsg(LOG_DEBUG, "Criterion (type: \"%s\", method: forbidden) %s. "
		       "The physical device is now locked into a failing grade.",
		       criterionName, met);
	    isFailureLocked = 1;
	  }
	  else
	    atlrLogMsg(LOG_DEBUG, "Criterion (type: \"%s\", method: forbidden) %s. "
		       "The current grade is %d.",
		       criterionName, met, grade);
	  break;
	}
      }
    }

    if (isFailureLocked)
      atlrLogMsg(LOG_INFO, "The physical device \"%s\" received a failing grade.", properties.deviceName);
    else
      atlrLogMsg(LOG_INFO, "The physical device \"%s\" received a grade of %d.", properties.deviceName, grade);
    if (!isFailureLocked && ((grade > bestGrade) || !foundPhysicalDevice))
    {
      if (foundPhysicalDevice) deinitSwapchainSupportDetails(&device->swapchainSupportDetails);
      bestGrade = grade;
      foundPhysicalDevice = 1;
      device->physical = physical;
      device->queueFamilyIndices = queueFamilyIndices;
      device->swapchainSupportDetails = swapchainSupportDetails;
    }
    else deinitSwapchainSupportDetails(&swapchainSupportDetails);
  }
  free(physicalDevices);

  if (!foundPhysicalDevice)
  {
    atlrLogMsg(LOG_FATAL, "An appropriate physical device was never found.");
    return 0;
  }

  {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device->physical, &properties);
    atlrLogMsg(LOG_INFO, "With the highest grade of %d, the physical device \"%s\" was selected.",
	       bestGrade, properties.deviceName);
  }

  AtlrU32 uniqueQueueFamilyIndices[2];
  const AtlrQueueFamilyIndices* queueFamilyIndices = &device->queueFamilyIndices;
  uniqueQueueFamilyIndices[0] = queueFamilyIndices->graphics_index;
  AtlrU8 uniqueQueueFamilyIndicesCount = 1;
  if (queueFamilyIndices->graphics_index != queueFamilyIndices->present_index)
  {
    uniqueQueueFamilyIndices[1] = queueFamilyIndices->present_index;
    uniqueQueueFamilyIndicesCount++;
  }
  VkDeviceQueueCreateInfo queueInfos[2];
  const AtlrF32 priority = 1.0f;
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
  const VkDeviceCreateInfo deviceInfo =
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .queueCreateInfoCount = uniqueQueueFamilyIndicesCount,
    .pQueueCreateInfos = queueInfos,
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = &swapchainExtension
  };
  if (vkCreateDevice(device->physical, &deviceInfo, instance->allocator, &device->logical) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_FATAL, "vkCreateDevice did return VK_SUCCESS.");
    deinitSwapchainSupportDetails(&device->swapchainSupportDetails);
    return 0;
  }

  vkGetDeviceQueue(device->logical, queueFamilyIndices->graphics_index, 0, &device->graphicsQueue);
  vkGetDeviceQueue(device->logical, queueFamilyIndices->present_index, 0, &device->presentQueue);

  atlrLogMsg(LOG_INFO, "Done initializing antler device.");
  return 1;
}

void atlrDeinitDevice(AtlrDevice* restrict device, const AtlrInstance* restrict instance)
{
  atlrLogMsg(LOG_INFO, "Deinitializing antler device ...");

  vkDestroyDevice(device->logical, instance->allocator);
  deinitSwapchainSupportDetails(&device->swapchainSupportDetails);

  atlrLogMsg(LOG_INFO, "Done deinitializing antler device.");
}
