#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

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

typedef enum
{
  ATLR_MODE_HEADLESS,
  ATLR_MODE_GLFW
  
} AtlrMode;

typedef struct AtlrInstance
{
  AtlrMode mode;
  VkInstance instance;
  VkAllocationCallbacks* allocator;
  VkSurfaceKHR surface;
  void* data;
  
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
  // Vulkan version criteria
  ATLR_DEVICE_CRITERION_VULKAN_VERSION_AT_LEAST_1_1,
  ATLR_DEVICE_CRITERION_VULKAN_VERSION_AT_LEAST_1_2,
  ATLR_DEVICE_CRITERION_VULKAN_VERSION_AT_LEAST_1_3,
  
  // these criteria correspond to the Vulkan physical device types
  ATLR_DEVICE_CRITERION_OTHER_PHYSICAL_DEVICE,
  ATLR_DEVICE_CRITERION_INTEGRATED_GPU_PHYSICAL_DEVICE,
  ATLR_DEVICE_CRITERION_DISCRETE_GPU_PHYSICAL_DEVICE,
  ATLR_DEVICE_CRITERION_VIRTUAL_GPU_PHYSICAL_DEVICE,
  ATLR_DEVICE_CRITERION_CPU_PHYSICAL_DEVICE,

  // queue criteria
  ATLR_DEVICE_CRITERION_QUEUE_FAMILY_GRAPHICS_SUPPORT,
  ATLR_DEVICE_CRITERION_QUEUE_FAMILY_PRESENT_SUPPORT,

  // swapchain criterion
  ATLR_DEVICE_CRITERION_SWAPCHAIN_SUPPORT,

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
  AtlrI32 pointShift;
  
} AtlrDeviceCriterion;

typedef AtlrDeviceCriterion AtlrDeviceCriteria[ATLR_DEVICE_CRITERION_TOT];

typedef struct AtlrDevice
{
  VkPhysicalDevice physical;
  AtlrQueueFamilyIndices queueFamilyIndices;
  AtlrU8 hasSwapchainSupport;
  AtlrSwapchainSupportDetails swapchainSupportDetails;
  VkDevice logical;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  
} AtlrDevice;

typedef struct AtlrBuffer
{
  VkBuffer buffer;
  VkDeviceMemory memory;
  void* data;
  
} AtlrBuffer;

typedef struct AtlrSingleRecordCommandContext
{
  VkCommandPool commandPool;
  VkFence fence;
  
} AtlrSingleRecordCommandContext;

//
// Functions
//

void atlrLogMsg(AtlrLoggerType, const char* restrict format, ...);

AtlrU8 atlrGetVulkanMemoryTypeIndex(AtlrU32* restrict index, const VkPhysicalDevice physical, const AtlrU32 typeFilter, const VkMemoryPropertyFlags properties);

AtlrU8 atlrInitInstanceHeadless(AtlrInstance* restrict, const char* restrict name);
void atlrDeinitInstanceHeadless(AtlrInstance* restrict);
AtlrU8 atlrInitInstanceGLFW(AtlrInstance* restrict, const int width, const int height, const char* restrict name);
void atlrDeinitInstanceGLFW(AtlrInstance* restrict);

void atlrInitDeviceCriteria(AtlrDeviceCriterion* restrict);
AtlrU8 atlrSetDeviceCriterion(AtlrDeviceCriterion* restrict criteria, AtlrDeviceCriterionType, AtlrDeviceCriterionMethod, AtlrI32 pointShift);
AtlrU8 atlrInitDevice(AtlrDevice* restrict, const AtlrInstance* restrict, AtlrDeviceCriterion* restrict);
void atlrDeinitDevice(AtlrDevice* restrict, const AtlrInstance* restrict);

AtlrU8 atlrInitGraphicsCommandPool(VkCommandPool* restrict, const VkCommandPoolCreateFlags, const AtlrInstance* restrict, const AtlrDevice* restrict);
void atlrDeinitCommandPool(const VkCommandPool, const AtlrInstance* restrict, const AtlrDevice* restrict);
AtlrU8 atlrAllocatePrimaryCommandBuffers(VkCommandBuffer* restrict commandBuffers, AtlrU32 commandBufferCount, const VkCommandPool, const AtlrDevice*);
AtlrU8 atlrBeginCommandRecording(const VkCommandBuffer, const VkCommandBufferUsageFlags);
AtlrU8 atlrEndCommandRecording(const VkCommandBuffer);
AtlrU8 atlrInitSingleRecordCommandContext(AtlrSingleRecordCommandContext* restrict, const AtlrInstance* restrict, const AtlrDevice* restrict);
void atlrDeinitSingleRecordCommandContext(AtlrSingleRecordCommandContext* restrict, const AtlrInstance* restrict, const AtlrDevice* restrict);
AtlrU8 atlrBeginSingleRecordCommands(VkCommandBuffer* restrict, const AtlrSingleRecordCommandContext* restrict, const AtlrDevice*);
AtlrU8 atlrEndSingleRecordCommands(const VkCommandBuffer, const AtlrSingleRecordCommandContext* restrict, const AtlrDevice*);
void atlrCommandSetViewport(const VkCommandBuffer, const float width, const float height);
void atlrCommandSetScissor(const VkCommandBuffer, const VkExtent2D*);

AtlrU8 atlrInitBuffer(AtlrBuffer* restrict, const AtlrU64 size, const VkBufferUsageFlags, const VkMemoryPropertyFlags,
		      const AtlrInstance*, const AtlrDevice*);
void atlrDeinitBuffer(AtlrBuffer* restrict, const AtlrInstance*, const AtlrDevice*);
AtlrU8 atlrMapBuffer(AtlrBuffer* restrict, const AtlrU64 offset, const AtlrU64 size, const VkMemoryMapFlags,
		     const AtlrDevice*);
void atlrUnmapBuffer(const AtlrBuffer* restrict, const AtlrDevice*);
AtlrU8 atlrLoadBuffer(AtlrBuffer* restrict, const AtlrU64 offset, const AtlrU64 size, const VkMemoryMapFlags, const void* restrict data,
		      const AtlrDevice* restrict);
AtlrU8 atlrCopyBuffer(const AtlrBuffer* restrict dst, const AtlrBuffer* restrict src, const AtlrU64 dstOffset, const AtlrU64 srcOffset, const AtlrU64 size,
		      const AtlrDevice* restrict, const AtlrSingleRecordCommandContext*);
