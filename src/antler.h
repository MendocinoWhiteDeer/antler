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
  ATLR_LOG_TRACE,
  ATLR_LOG_DEBUG,
  ATLR_LOG_INFO,
  ATLR_LOG_WARN,
  ATLR_LOG_ERROR,
  ATLR_LOG_FATAL
  
} AtlrLoggerType;

typedef uint8_t  AtlrU8;
typedef uint16_t AtlrU16;
typedef uint32_t AtlrU32;
typedef uint64_t AtlrU64;
typedef int8_t   AtlrI8;
typedef int16_t  AtlrI16;
typedef int32_t  AtlrI32;
typedef int64_t  AtlrI64;

typedef enum
{
  ATLR_MODE_HOST_HEADLESS,
  ATLR_MODE_HOST_GLFW,
  ATLR_MODE_HOOK
  
} AtlrMode;

typedef struct _AtlrInstance
{
  AtlrMode mode;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkAllocationCallbacks* allocator;
  VkSurfaceKHR surface;
  void* data;
  
} AtlrInstance;

typedef struct _AtlrQueueFamilyIndices
{
  AtlrU8 isGraphicsCompute;
  AtlrU8 isPresent;
  AtlrU32 graphicsComputeIndex;
  AtlrU32 presentIndex;
  
} AtlrQueueFamilyIndices;

typedef struct _AtlrSwapchainSupportDetails
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
  ATLR_DEVICE_CRITERION_QUEUE_FAMILY_COMPUTE_SUPPORT,

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

typedef struct _AtlrDeviceCriterion
{
  AtlrDeviceCriterionMethod method;
  AtlrI32 pointShift;
  
} AtlrDeviceCriterion;

typedef AtlrDeviceCriterion AtlrDeviceCriteria[ATLR_DEVICE_CRITERION_TOT];

typedef struct _AtlrDevice
{
  const AtlrInstance* instance;
  VkPhysicalDevice physical;
  AtlrQueueFamilyIndices queueFamilyIndices;
  AtlrU8 hasSwapchainSupport;
  AtlrSwapchainSupportDetails swapchainSupportDetails;
  VkDevice logical;
  VkQueue graphicsComputeQueue;
  VkQueue presentQueue;
  
} AtlrDevice;

typedef struct _AtlrSingleRecordCommandContext
{
  const AtlrDevice* device;
  VkQueue queue;
  VkCommandPool commandPool;
  VkFence fence;
  
} AtlrSingleRecordCommandContext;

typedef struct _AtlrBuffer
{
  const AtlrDevice* device;
  VkBuffer buffer;
  VkDeviceMemory memory;
  void* data;
  
} AtlrBuffer;

typedef struct _AtlrMesh
{
  AtlrBuffer vertexBuffer;
  AtlrBuffer indexBuffer;
  AtlrU32 indexCount;
  
} AtlrMesh;

typedef struct _AtlrImage
{
  const AtlrDevice* device;
  VkImage image;
  VkDeviceMemory memory;
  VkImageView imageView;
  VkFormat format;
  AtlrU32 width;
  AtlrU32 height;
  AtlrU32 layerCount;
  
} AtlrImage;

typedef struct _AtlrDescriptorSetLayout
{
  const AtlrDevice* device;
  VkDescriptorSetLayout layout;
  
} AtlrDescriptorSetLayout;

typedef struct _AtlrDescriptorPool
{
  const AtlrDevice* device;
  VkDescriptorPool pool;
  
} AtlrDescriptorPool;

typedef struct _AtlrRenderPass
{
  const AtlrDevice* device;
  VkRenderPass renderPass;
  AtlrU32 clearValueCount;
  VkClearValue* clearValues;
  
} AtlrRenderPass;

typedef struct _AtlrSwapchain
{
  const AtlrDevice* device;
  VkSwapchainKHR swapchain;
  VkExtent2D extent;
  VkFormat format;

  AtlrU32 imageCount;
  VkImage* images;
  VkImageView* imageViews;
  AtlrImage depthImage;

  AtlrRenderPass renderPass;
  VkFramebuffer* framebuffers;
  
} AtlrSwapchain;

typedef struct _AtlrFrame
{
  VkCommandBuffer commandBuffer;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;
  
} AtlrFrame;

typedef struct _AtlrFrameCommandContext
{
  AtlrU8 isResize;
  AtlrU32 imageIndex;
  AtlrSwapchain* swapchain;
  VkCommandPool commandPool;
  AtlrU8 currentFrame;
  AtlrU8 frameCount;
  AtlrFrame* frames;
  
} AtlrFrameCommandContext;

//
// Functions
//

// logger.c
void atlrLog(AtlrLoggerType, const char* restrict format, ...);
#define ATLR_ERROR_MSG(format, ...) atlrLog(ATLR_LOG_ERROR, "{Location: %s:%d}: " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define ATLR_FATAL_MSG(format, ...) atlrLog(ATLR_LOG_FATAL, "{Location: %s:%d}: " format, __FILE__, __LINE__, ##__VA_ARGS__) 

// util.c
AtlrU8 atlrGetVulkanMemoryTypeIndex(AtlrU32* restrict index, const VkPhysicalDevice physical, const AtlrU32 typeFilter, const VkMemoryPropertyFlags properties);

// instance.c
AtlrU8 atlrInitInstanceHostHeadless(AtlrInstance* restrict, const char* restrict name);
void atlrDeinitInstanceHostHeadless(const AtlrInstance* restrict);
AtlrU8 atlrInitInstanceHostGLFW(AtlrInstance* restrict, const int width, const int height, const char* restrict name);
void atlrDeinitInstanceHostGLFW(const AtlrInstance* restrict);

// device.c
AtlrU8 atlrInitSwapchainSupportDetails(AtlrSwapchainSupportDetails* restrict, const AtlrInstance* restrict, const VkPhysicalDevice);
void atlrDeinitSwapchainSupportDetails(AtlrSwapchainSupportDetails* restrict);
void atlrInitDeviceCriteria(AtlrDeviceCriterion* restrict);
AtlrU8 atlrSetDeviceCriterion(AtlrDeviceCriterion* restrict criteria, AtlrDeviceCriterionType, AtlrDeviceCriterionMethod, AtlrI32 pointShift);
AtlrU8 atlrInitDeviceHost(AtlrDevice* restrict, const AtlrInstance* restrict, const AtlrDeviceCriterion* restrict);
void atlrDeinitDeviceHost(AtlrDevice* restrict);

// commands.c
AtlrU8 atlrInitCommandPool(VkCommandPool* restrict, const VkCommandPoolCreateFlags, const AtlrU32 queueFamilyIndex,
				   const AtlrDevice* restrict);
void atlrDeinitCommandPool(const VkCommandPool,
			   const AtlrDevice* restrict);
AtlrU8 atlrAllocatePrimaryCommandBuffers(VkCommandBuffer* restrict commandBuffers, AtlrU32 commandBufferCount, const VkCommandPool,
					 const AtlrDevice*);
AtlrU8 atlrBeginCommandRecording(const VkCommandBuffer, const VkCommandBufferUsageFlags);
AtlrU8 atlrEndCommandRecording(const VkCommandBuffer);
AtlrU8 atlrInitSingleRecordCommandContext(AtlrSingleRecordCommandContext* restrict, const AtlrU32 queueFamilyIndex,
					  const AtlrDevice* restrict);
void atlrDeinitSingleRecordCommandContext(AtlrSingleRecordCommandContext* restrict);
AtlrU8 atlrBeginSingleRecordCommands(VkCommandBuffer* restrict, const AtlrSingleRecordCommandContext* restrict);
AtlrU8 atlrEndSingleRecordCommands(const VkCommandBuffer, const AtlrSingleRecordCommandContext* restrict);
void atlrCommandSetViewport(const VkCommandBuffer, const float width, const float height);
void atlrCommandSetScissor(const VkCommandBuffer, const VkExtent2D*);
AtlrU8 atlrInitFrameCommandContextHostGLFW(AtlrFrameCommandContext* restrict, const AtlrU8 frameCount,
				   AtlrSwapchain* restrict);
void atlrDeinitFrameCommandContextHostGLFW(AtlrFrameCommandContext* restrict);
AtlrU8 atlrBeginFrameCommandsHostGLFW(AtlrFrameCommandContext* restrict);
AtlrU8 atlrEndFrameCommandsHostGLFW(AtlrFrameCommandContext* restrict);
VkCommandBuffer atlrGetFrameCommandContextCommandBufferHostGLFW(const AtlrFrameCommandContext* restrict);

// buffer.c
AtlrU8 atlrInitBuffer(AtlrBuffer* restrict, const AtlrU64 size, const VkBufferUsageFlags, const VkMemoryPropertyFlags,
		      const AtlrDevice*);
AtlrU8 atlrInitStagingBuffer(AtlrBuffer* restrict, const AtlrU64 size,
		      const AtlrDevice*);
AtlrU8 atlrInitReadbackingBuffer(AtlrBuffer* restrict, const AtlrU64 size,
		      const AtlrDevice*);
void atlrDeinitBuffer(AtlrBuffer* restrict);
AtlrU8 atlrMapBuffer(AtlrBuffer* restrict, const AtlrU64 offset, const AtlrU64 size, const VkMemoryMapFlags);
void atlrUnmapBuffer(const AtlrBuffer* restrict);
AtlrU8 atlrWriteBuffer(AtlrBuffer* restrict, const AtlrU64 offset, const AtlrU64 size, const VkMemoryMapFlags, const void* restrict data);
AtlrU8 atlrReadBuffer(AtlrBuffer* restrict, const AtlrU64 offset, const AtlrU64 size, const VkMemoryMapFlags flags, void* restrict data);
AtlrU8 atlrCopyBuffer(const AtlrBuffer* restrict dst, const AtlrBuffer* restrict src, const AtlrU64 dstOffset, const AtlrU64 srcOffset, const AtlrU64 size,
		      const AtlrSingleRecordCommandContext* restrict);
AtlrU8 atlrCopyBufferToImage(const AtlrBuffer*, const AtlrImage* restrict,
			     const VkOffset2D*, const VkExtent2D*,
			     const AtlrSingleRecordCommandContext* restrict);
AtlrU8 atlrStageBuffer(AtlrBuffer* restrict, const AtlrU64 offset, const AtlrU64 size, const void* restrict data,
		       const AtlrSingleRecordCommandContext* restrict);
AtlrU8 atlrReadbackBuffer(AtlrBuffer* restrict, const AtlrU64 offset, const AtlrU64 size, void* restrict data,
			  const AtlrSingleRecordCommandContext* restrict);
AtlrU8 atlrInitMesh(AtlrMesh* restrict, const AtlrU64 verticesSize, const void* restrict vertices, const AtlrU32 indexCount, const AtlrU16* restrict indices,
		    const AtlrDevice* restrict device, const AtlrSingleRecordCommandContext* restrict);
void atlrDeinitMesh(AtlrMesh* restrict mesh);
void atlrBindMesh(const AtlrMesh* restrict mesh, const VkCommandBuffer);
void atlrDrawMesh(const AtlrMesh* restrict mesh, const VkCommandBuffer);

// image.c
VkImageView atlrInitImageView(const VkImage,
			      const VkImageViewType, const VkFormat, const VkImageAspectFlags, const AtlrU32 layerCount,
			      const AtlrDevice* restrict);
void atlrDeinitImageView(const VkImageView,
			 const AtlrDevice* restrict);
AtlrU8 atlrInitImage(AtlrImage* restrict, const AtlrU32 width, const AtlrU32 height,
		     const AtlrU32 layerCount, const VkFormat, const VkImageTiling, const VkImageUsageFlags,
		     const VkMemoryPropertyFlags, const VkImageViewType, const VkImageAspectFlags,
		     const AtlrDevice* restrict);
void atlrDeinitImage(const AtlrImage* restrict);
AtlrU8 atlrIsValidDepthImage(const AtlrImage* restrict);
AtlrU8 atlrInitDepthImage(AtlrImage* restrict, const AtlrU32 width, const AtlrU32 height,
			  const AtlrDevice* restrict);
void atlrDeinitDepthImage(const AtlrImage* restrict);
AtlrU8 atlrTransitionImageLayout(const AtlrImage* restrict, const VkImageLayout oldLayout, const VkImageLayout newLayout,
				 const AtlrSingleRecordCommandContext* restrict);

// descriptor.c
VkDescriptorSetLayoutBinding atlrInitDescriptorSetLayoutBinding(const AtlrU32 binding, const VkDescriptorType, const VkShaderStageFlags);
AtlrU8 atlrInitDescriptorSetLayout(AtlrDescriptorSetLayout* restrict, const AtlrU32 bindingCount, const VkDescriptorSetLayoutBinding* restrict,
				   const AtlrDevice* restrict);
void atlrDeinitDescriptorSetLayout(const AtlrDescriptorSetLayout* restrict);
VkDescriptorPoolSize atlrInitDescriptorPoolSize(const VkDescriptorType, const AtlrU32 descriptorCount);
AtlrU8 atlrInitDescriptorPool(AtlrDescriptorPool* restrict, const AtlrU32 maxSets, const AtlrU32 poolSizeCount, const VkDescriptorPoolSize* restrict,
			      const AtlrDevice* restrict);
void atlrDeinitDescriptorPool(const AtlrDescriptorPool* restrict);
AtlrU8 atlrAllocDescriptorSets(const AtlrDescriptorPool* restrict, const AtlrU32 setCount, const VkDescriptorSetLayout* restrict setLayouts, VkDescriptorSet* restrict sets);
VkDescriptorBufferInfo atlrInitDescriptorBufferInfo(const AtlrBuffer* restrict, const AtlrU64 size);
VkWriteDescriptorSet atlrWriteBufferDescriptorSet(const VkDescriptorSet, const AtlrU32 binding, const VkDescriptorType, const VkDescriptorBufferInfo* restrict);
VkDescriptorImageInfo atlrInitDescriptorImageInfo(const AtlrImage* restrict, const VkSampler, const VkImageLayout);
VkWriteDescriptorSet atlrWriteImageDescriptorSet(const VkDescriptorSet, const AtlrU32 binding, const VkDescriptorType, const VkDescriptorImageInfo* restrict);

// pipeline.c
VkShaderModule atlrInitShaderModule(const char* restrict path, const AtlrDevice* restrict);
void atlrDeinitShaderModule(const VkShaderModule module, const AtlrDevice* restrict);
VkPipelineShaderStageCreateInfo atlrInitPipelineVertexShaderStageInfo(const VkShaderModule);
VkPipelineShaderStageCreateInfo atlrInitPipelineFragmentShaderStageInfo(const VkShaderModule);
VkPipelineShaderStageCreateInfo atlrInitPipelineComputeShaderStageInfo(const VkShaderModule);
VkPipelineInputAssemblyStateCreateInfo atlrInitPipelineInputAssemblyStateInfo();
VkPipelineViewportStateCreateInfo atlrInitPipelineViewportStateInfo();
VkPipelineRasterizationStateCreateInfo atlrInitPipelineRasterizationStateInfo();
VkPipelineMultisampleStateCreateInfo atlrInitPipelineMultisampleStateInfo();
VkPipelineDepthStencilStateCreateInfo atlrInitPipelineDepthStencilStateInfo();
VkPipelineColorBlendAttachmentState atlrInitPipelineColorBlendAttachmentState();
VkPipelineColorBlendStateCreateInfo atlrInitPipelineColorBlendStateInfo(const VkPipelineColorBlendAttachmentState* restrict);
VkPipelineDynamicStateCreateInfo atlrInitPipelineDynamicStateInfo();
VkPipelineLayoutCreateInfo atlrInitPipelineLayoutInfo(AtlrU32 setLayoutCount, const VkDescriptorSetLayout* restrict);

// render-pass.c
VkAttachmentDescription atlrGetColorAttachmentDescription(const VkFormat, const VkImageLayout finalLayout);
VkAttachmentDescription atlrGetDepthAttachmentDescription(const AtlrImage* restrict);
AtlrU8 atlrInitRenderPass(AtlrRenderPass* restrict,
			  const AtlrU32 colorAttachmentCount, const VkAttachmentDescription* restrict colorAttachment,
			  const VkAttachmentDescription* restrict depthAttachment,
			  const AtlrDevice* restrict);
void atlrDeinitRenderPass(const AtlrRenderPass* restrict);
void atlrBeginRenderPass(const AtlrRenderPass* restrict,
			 const VkCommandBuffer, const VkFramebuffer, const VkExtent2D* restrict);
void atlrEndRenderPass(const VkCommandBuffer);

// swapchain.c
AtlrU8 atlrInitSwapchainHostGLFW(AtlrSwapchain* restrict, const AtlrU8 initRenderPass,
				 const AtlrDevice* restrict);
void atlrDeinitSwapchainHostGLFW(AtlrSwapchain* restrict, const AtlrU8 deinitRenderPass);
AtlrU8 atlrReinitSwapchainHostGLFW(AtlrSwapchain* restrict swapchain);
VkResult atlrNextSwapchainImage(const AtlrSwapchain* restrict, const VkSemaphore imageAvailableSemaphore, AtlrU32* imageIndex);
VkResult atlrSwapchainSubmit(const AtlrSwapchain* restrict, const VkCommandBuffer,
			     const VkSemaphore imageAvailableSemaphore, const VkSemaphore renderFinishedSemaphore, const VkFence);
VkResult atlrSwapchainPresent(const AtlrSwapchain* restrict, const VkSemaphore renderFinishedSemaphore, const AtlrU32* restrict imageIndex);
