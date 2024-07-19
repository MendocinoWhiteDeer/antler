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

#ifdef ATLR_DEBUG

static const char* validationLayers[] =
{
  "VK_LAYER_KHRONOS_validation"
};

#endif

static AtlrU32 vulkanVersion = VK_API_VERSION_1_3;

#ifdef ATLR_DEBUG

static AtlrU8 areValidationLayersSupported()
{
  AtlrU32 availableLayerCount = 0;
  if(vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_FATAL, "vkEnumerateInstanceLayerProperties (first call) did not return VK_SUCCESS.");
      return 0;
    }
  VkLayerProperties availableLayers[availableLayerCount];
  if(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_FATAL, "vkEnumerateInstanceLayerProperties (second call) did not return VK_SUCCESS.");
      return 0;
    }

  const AtlrU32 requiredLayerCount = sizeof(validationLayers) / sizeof(const char*); 
  for (AtlrU8 i = 0; i < sizeof(validationLayers) / sizeof(const char*); i++)
    {
      AtlrU8 layerFound = 0;
      for (AtlrU32 j = 0; j < availableLayerCount; j++)
	{
	  if (!strcmp(validationLayers[i], availableLayers[j].layerName))
	    {
	      layerFound = 1;
	      break;
	    }
	}

      if (!layerFound)
	{
	  atlrLogMsg(LOG_FATAL, "Required Vulkan validation layer \"%s\" is unavailable.", validationLayers[i]);
	  return 0;
	}
    }
  atlrLogMsg(LOG_DEBUG, "All required Vulkan validation layers are available.");

  return 1;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	        VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
    		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData)
{
  const char* str = "{VULKAN} \n%s";
  switch(msgSeverity)
  {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    atlrLogMsh(LOG_TRACE, str, callbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    atlrLogMsg(LOG_INFO, str, callbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    atlrLogMsg(LOG_WARN, str, callbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    atlrLogMsg(LOG_ERROR, str, callbackData->pMessage);
    break;
  default:
    atlrLogMsg(LOG_ERROR, "Invalid value \"%d\" for the message severity in the Vulkan debug callback.", msgSeverity);
  }
  return VK_FALSE;
}

static void initVkDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT* restrict info)
{
  info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  info->pNext = 0;
  info->flags = 0;
  info->messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  info->messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  info->pfnUserCallback = debugCallback;
}

static AtlrU8 initDebugMessenger(AtlrInstance* restrict instance)
{
  VkDebugUtilsMessengerCreateInfoEXT debugInfo;
  initVkDebugUtilsMessengerCreateInfoEXT(&debugInfo);

  PFN_vkCreateDebugUtilsMessengerEXT msgFn =
    (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->handle, "vkCreateDebugUtilsMessengerEXT");
  if (!msgFn)
  {
    atlrLogMsg(LOG_FATAL, "vkGetInstanceProcAddr returned 0.");
    return 0;
  }
  if (msgFn(instance->handle, &debugInfo, instance->allocator, instance->debugMessenger != VK_SUCCESS)
    return 0;

  return 1;
}

static void deinitDebugMessenger(const AtlrInstance* restrict instance)
{
  PFN_vkDestroyDebugUtilsMessengerEXT msgFn =
    (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->handle, "vkDestroyDebugUtilsMessengerEXT");
  if (!msgFn)
    atlrLogMsg(LOG_ERROR, "vkGetInstanceProcAddr returned 0.");
  msgFn(instance->handle, instance->debugMessenger, instance->allocator);
}

#endif

AtlrU8 initAtlrInstance(AtlrInstance* restrict instance,
			const int width, const int height, const char* restrict name)
{
  if (!glfwInit())
  {
    atlrLogMsg(LOG_FATAL, "glfwInit returned GLFW_FALSE.");
    return 0;
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  instance->window = glfwCreateWindow(width, height, name, 0, 0);
  if (!instance->window)
  {
    atlrLogMsg(LOG_FATAL, "glfwCreateWindow returned 0.");
    return 0;
  }
  
#ifdef ATLR_DEBUG
  if (!areValidationLayersSupported())
  {
    atlrLogMsg(LOG_FATAL, "areValidationLayersSupported returned 0.");
    return 0;
  }
#endif

  if (vkEnumerateInstanceVersion(&vulkanVersion) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_FATAL, "Vulkan version 1.3 is not supported.");
    return 0;
  }

  const VkApplicationInfo appInfo =
  {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = 0,
    .pApplicationName = "antler",
    .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
    .pEngineName = "none",
    .engineVersion = VK_MAKE_VERSION(0, 0, 1),
    .apiVersion = vulkanVersion
  };

  VkInstanceCreateInfo instanceInfo =
  {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo
  };

  AtlrU32 extensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
  const char** extensions = malloc((extensionCount + 1) * sizeof(const char*));
  for (AtlrU32 i = 0; i < extensionCount; i++) extensions[i] = glfwExtensions[i];
#ifdef ATLR_DEBUG
  extensions[extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  entensionCount++;
#endif
  for (AtlrU32 i = 0; i < extensionCount; i++)
    atlrLogMsg(LOG_DEBUG, "Requiring Vulkan instance extension \"%s\"", extensions[i]);
  instanceInfo.enabledExtensionCount = extensionCount;
  instanceInfo.ppEnabledExtensionNames = extensions; 

#ifdef ATLR_DEBUG
  instanceInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(const char*);
  instanceInfo.ppEnabledLayerNames = validationLayers;

  VkDebugUtilsMessengerCreateInfoEXT debugInfo;
  initVkDebugUtilsMessengerCreateInfoEX(&debugInfo);
  instanceInfo.pNext = &debugInfo;
#else
  instanceInfo.enabledLayerCount = 0;
  instanceInfo.ppEnabledLayerNames = 0;
  instanceInfo.pNext = 0;
#endif

  if (vkCreateInstance(&instanceInfo, instance->allocator, &instance->handle) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_FATAL, "vkCreateInstance did not return VK_SUCCESS.");
    return 0;
  }

  // clean
  free(extensions);

  return 1;
}

 void deinitAtlrInstance(AtlrInstance* restrict instance)
 {
   glfwTerminate();
   glfwDestroyWindow(instance->window);
#ifdef ATLR_DEBUG
   deinitDebugMessenger(instance);
#endif
   vkDestroyInstance(instance->handle, instance->allocator);
 }
