#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"

///
/// Data types
///

typedef enum
{
  LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
  
} AtlrLoggerType;

typedef uint8_t  AtlrU8;
typedef uint16_t AtlrU16;
typedef uint32_t AtlrU32;
typedef uint64_t AtlrU64;
typedef int8_t   AtlrI8;
typedef int16_t  AtlrI16;
typedef int32_t  AtlrI32;
typedef int64_t  AtlrI64;
typedef float    AtlrF32;
typedef double   AtlrF64;

typedef struct AtlrInstance
{
  VkInstance handle;
  GLFWwindow* window;
  VkAllocationCallbacks* allocator;
  VkSurfaceKHR surface;
  AtlrU32 validationLayerCount;
  const char** validationLayers;
  
} AtlrInstance;

typedef struct AtlrQueueFamilyIndices
{
  AtlrU8 isGraphics;
  AtlrU8 isPresent;
  AtlrU32 graphics_index;
  AtlrU32 present_index;
  
} AtlrQueueFamilyIndices;

typedef struct AtlrSwapchainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  AtlrU32 formatCount;
  VkSurfaceFormatKHR* formats;
  AtlrU32 presentModeCount;
  VkPresentModeKHR* presentModes;
  
} AtlrSwapchainSupportDetails;

typedef enum
{
  // These criteria correspond to the Vulkan physical device types
  ATLR_DEVICE_CRITERION_OTHER_PHYSICAL_DEVICE,
  ATLR_DEVICE_CRITERION_INTEGRATED_GPU_PHYSICAL_DEVICE,
  ATLR_DEVICE_CRITERION_DISCRETE_GPU_PHYSICAL_DEVICE,
  ATLR_DEVICE_CRITERION_VIRTUAL_GPU_PHYSICAL_DEVICE,
  ATLR_DEVICE_CRITERION_CPU_PHYSICAL_DEVICE,

  ATLR_DEVICE_CRITERION_TOT
  
} AtlrDeviceCriterionType;

typedef enum
{
  ATLR_DEVICE_CRITERION_METHOD_POINT_SHIFT,
  ATLR_DEVICE_CRITERION_METHOD_REQUIRED,
  ATLR_DEVICE_CRITERION_METHOD_FORBIDDEN
  
} AtlrDeviceCriterionMethod;

typedef struct AtlrDeviceCriterion
{
  AtlrDeviceCriterionMethod method;
  AtlrI32 point_shift;
  
} AtlrDeviceCriterion;

typedef AtlrDeviceCriterion AtlrDeviceCriteria[ATLR_DEVICE_CRITERION_TOT];

typedef struct AtlrDevice
{
  VkPhysicalDevice physical;
  AtlrQueueFamilyIndices queueFamilyIndices;
  AtlrSwapchainSupportDetails swapchainSupportDetails;
  VkDevice logical;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  
} AtlrDevice;

//
// Functions
//

void atlrLogMsg(AtlrLoggerType, const char* restrict format, ...);

AtlrU8 initAtlrInstance(AtlrInstance* restrict,
			const int width, const int height, const char* restrict name);
void deinitAtlrInstance(AtlrInstance* restrict);

void initAtlrDeviceCriteria(AtlrDeviceCriterion* restrict);
AtlrU8 setAtlrDeviceCriterion(AtlrDeviceCriterion* restrict, AtlrDeviceCriterionType, AtlrDeviceCriterionMethod, AtlrI32 point_shift);
AtlrU8 initAtlrDevice(AtlrDevice* restrict, const AtlrInstance* restrict, AtlrDeviceCriterion* restrict);
void deinitAtlrDevice(AtlrDevice* restrict, const AtlrInstance* restrict);
