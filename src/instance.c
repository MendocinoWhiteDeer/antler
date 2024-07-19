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

static VkDebugUtilsMessengerEXT debugMessenger; // originally this was wrapped in a struct in a header file; DON'T DO THAT, CAUSES SEGFAULT!!
static const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

static AtlrU8 isValidationLayerAvailable()
{
  AtlrU32 availableLayerCount = 0;
  if(vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_FATAL, "vkEnumerateInstanceLayerProperties (first call) did not return VK_SUCCESS.");
      return 0;
    }
  VkLayerProperties* availableLayers = malloc(availableLayerCount * sizeof(VkLayerProperties));
  if(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_FATAL, "vkEnumerateInstanceLayerProperties (second call) did not return VK_SUCCESS.");
      free(availableLayers);
      return 0;
    }

  for (AtlrU32 i = 0; i < availableLayerCount; i++)
      if (!strcmp(validationLayerName, availableLayers[i].layerName))
	{
	  free(availableLayers);
	  return 1;
	}

  free(availableLayers);
  return 0;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	        VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
    		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData)
{
  const char* str = "{VULKAN} %s";
  switch(msgSeverity)
  {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      atlrLogMsg(LOG_TRACE, str, callbackData->pMessage);
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
  info->pNext = NULL;
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
  info->pUserData = NULL;
}

static AtlrU8 initDebugMessenger(AtlrInstance* restrict instance, const VkDebugUtilsMessengerCreateInfoEXT* restrict debugInfo)
{
  PFN_vkCreateDebugUtilsMessengerEXT pfnCreate =
    (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->handle, "vkCreateDebugUtilsMessengerEXT");
  if (!pfnCreate)
  {
    atlrLogMsg(LOG_FATAL, "vkGetInstanceProcAddr returned 0.");
    return 0;
  }
  if (pfnCreate(instance->handle, debugInfo, instance->allocator, &debugMessenger) != VK_SUCCESS)
    return 0;

  return 1;
}

static void deinitDebugMessenger(AtlrInstance* restrict instance)
{
  PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroy =
    (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->handle, "vkDestroyDebugUtilsMessengerEXT");
  if (!pfnDestroy)
    atlrLogMsg(LOG_ERROR, "vkGetInstanceProcAddr returned 0.");
  pfnDestroy(instance->handle, debugMessenger, instance->allocator);
}

#endif

static AtlrU8 areInstanceExtensionsAvailable(const char** restrict extensions, const AtlrU32 extensionCount)
{
  AtlrU32 availableExtensionCount = 0;
  if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_FATAL, "vkEnumerateInstanceExtensionProperties (first call) did not return VK_SUCCESS.");
      return 0;
    }
  VkExtensionProperties* availableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
  if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS)
    {
      atlrLogMsg(LOG_FATAL, "vkEnumerateInstanceExtensionProperties (second call) did not return VK_SUCCESS.");
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
	  atlrLogMsg(LOG_DEBUG, "Vulkan instance extension \"%s\" is available.", extensions[i]);
	  break;
	}
    if (!found)
      {
	atlrLogMsg(LOG_DEBUG, "Vulkan instance extension \"%s\" is unavailable.", extensions[i]);
	extensionsFound = 0;
      }
  }

  free(availableExtensions);
  return extensionsFound;
}

AtlrU8 initAtlrInstance(AtlrInstance* restrict instance,
			const int width, const int height, const char* restrict name)
{
  atlrLogMsg(LOG_INFO, "Initializing antler instance ...");
  
  if (!glfwInit())
  {
    atlrLogMsg(LOG_FATAL, "glfwInit returned GLFW_FALSE.");
    return 0;
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  instance->window = glfwCreateWindow(width, height, name, NULL, NULL);
  if (!instance->window)
  {
    atlrLogMsg(LOG_FATAL, "glfwCreateWindow returned 0.");
    return 0;
  }

  AtlrU32 apiVersion = VK_API_VERSION_1_3;
  if (vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_FATAL, "Vulkan version 1.3 is not supported.");
    return 0;
  }

  const VkApplicationInfo appInfo =
  {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = name,
    .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
    .pEngineName = "antler",
    .engineVersion = VK_MAKE_VERSION(0, 0, 1),
    .apiVersion = apiVersion
  };

  VkInstanceCreateInfo instanceInfo = {};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pApplicationInfo = &appInfo;

  // validation layer
#ifdef ATLR_DEBUG
  atlrLogMsg(LOG_DEBUG, "Requiring Vulkan validation layer.");
  if (!isValidationLayerAvailable())
  {
    atlrLogMsg(LOG_FATAL, "isValidationLayerAvailable returned 0.");
    return 0;
  }
  atlrLogMsg(LOG_DEBUG, "Vulkan validation layer is available.");
  instanceInfo.enabledLayerCount = 1;
  instanceInfo.ppEnabledLayerNames = &validationLayerName;

  VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
  initVkDebugUtilsMessengerCreateInfoEXT(&debugInfo);
  instanceInfo.pNext = &debugInfo;
#endif

  // extensions
  AtlrU32 extensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
  const char** extensions = malloc((extensionCount + 1) * sizeof(const char*));
  for (AtlrU32 i = 0; i < extensionCount; i++) extensions[i] = glfwExtensions[i];
#ifdef ATLR_DEBUG
  extensions[extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  extensionCount++;
#endif
  for (AtlrU32 i = 0; i < extensionCount; i++)
    atlrLogMsg(LOG_DEBUG, "Requiring Vulkan instance extension \"%s\".", extensions[i]);
  if (!areInstanceExtensionsAvailable(extensions, extensionCount))
  {
    atlrLogMsg(LOG_FATAL, "areInstanceExtensionsAvailable returned 0.");
    free(extensions);
    return 0;
  }
  instanceInfo.enabledExtensionCount = extensionCount;
  instanceInfo.ppEnabledExtensionNames = extensions; 

  // finish init instance
  if (vkCreateInstance(&instanceInfo, instance->allocator, &instance->handle) != VK_SUCCESS)
  {
    atlrLogMsg(LOG_FATAL, "vkCreateInstance did not return VK_SUCCESS.");
    return 0;
  }
#ifdef ATLR_DEBUG
  if (!initDebugMessenger(instance, &debugInfo))
  {
    atlrLogMsg(LOG_FATAL, "initDebugMessenger returned 0.");
    free(extensions);
    return 0;
  }
#endif

  free(extensions);
  atlrLogMsg(LOG_INFO, "Done initializing antler instance.");
  return 1;
}

 void deinitAtlrInstance(AtlrInstance* restrict instance)
 {
   atlrLogMsg(LOG_INFO, "Deinitializing antler ...");

#ifdef ATLR_DEBUG
   deinitDebugMessenger(instance);
#endif
   vkDestroyInstance(instance->handle, instance->allocator);
   
   glfwDestroyWindow(instance->window);
   glfwTerminate();
   
  atlrLogMsg(LOG_INFO, "Done deinitializing antler instance.");
 }
