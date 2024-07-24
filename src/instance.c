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

static AtlrU8 initAppInfo(VkApplicationInfo* appInfo, const char* name)
{
  AtlrU32 apiVersion = VK_API_VERSION_1_3;
  if (vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("Vulkan version 1.3 is not supported.");
    return 0;
  }

  appInfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo->pNext = NULL;
  appInfo->pApplicationName = name;
  appInfo->applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  appInfo->pEngineName = "antler";
  appInfo->engineVersion = VK_MAKE_VERSION(0, 0, 1);
  appInfo->apiVersion = apiVersion;

  return 1;
}

#ifdef ATLR_DEBUG

static const char* validationLayer = "VK_LAYER_KHRONOS_validation";

static AtlrU8 isValidationLayerAvailable()
{
  AtlrU32 availableLayerCount = 0;
  if(vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL) != VK_SUCCESS)
    {
      ATLR_LOG_ERROR("vkEnumerateInstanceLayerProperties (first call) did not return VK_SUCCESS.");
      return 0;
    }
  VkLayerProperties* availableLayers = malloc(availableLayerCount * sizeof(VkLayerProperties));
  if(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers) != VK_SUCCESS)
    {
      ATLR_LOG_ERROR("vkEnumerateInstanceLayerProperties (second call) did not return VK_SUCCESS.");
      free(availableLayers);
      return 0;
    }

  for (AtlrU32 i = 0; i < availableLayerCount; i++)
      if (!strcmp(validationLayer, availableLayers[i].layerName))
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
      atlrLog(LOG_TRACE, str, callbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      atlrLog(LOG_INFO, str, callbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      atlrLog(LOG_WARN, str, callbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      atlrLog(LOG_ERROR, str, callbackData->pMessage);
      break;
    default:
      atlrLog(LOG_ERROR, "Invalid value \"%d\" for the message severity in the Vulkan debug callback.", msgSeverity);
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
    (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->instance, "vkCreateDebugUtilsMessengerEXT");
  if (!pfnCreate)
  {
    ATLR_LOG_ERROR("vkGetInstanceProcAddr returned 0.");
    return 0;
  }
  if (pfnCreate(instance->instance, debugInfo, instance->allocator, &instance->debugMessenger) != VK_SUCCESS)
    return 0;

  return 1;
}

static void deinitDebugMessenger(const AtlrInstance* restrict instance)
{
  PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroy =
    (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->instance, "vkDestroyDebugUtilsMessengerEXT");
  if (!pfnDestroy)
    ATLR_LOG_ERROR("vkGetInstanceProcAddr returned 0.");
  pfnDestroy(instance->instance, instance->debugMessenger, instance->allocator);
}

#endif

static AtlrU8 areInstanceExtensionsAvailable(const char** restrict extensions, const AtlrU32 extensionCount)
{
  AtlrU32 availableExtensionCount = 0;
  if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("vkEnumerateInstanceExtensionProperties (first call) did not return VK_SUCCESS.");
    return 0;
  }
  if (!availableExtensionCount)
  {
    ATLR_LOG_ERROR("No available extensions.");
    return 0;
  }
  VkExtensionProperties* availableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
  if(vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensions) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("vkEnumerateInstanceExtensionProperties (second call) did not return VK_SUCCESS.");
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
	  atlrLog(LOG_DEBUG, "Vulkan instance extension \"%s\" is available.", extensions[i]);
	  break;
	}
    if (!found)
      {
	atlrLog(LOG_DEBUG, "Vulkan instance extension \"%s\" is unavailable.", extensions[i]);
	extensionsFound = 0;
      }
  }

  free(availableExtensions);
  return extensionsFound;
}

AtlrU8 atlrInitInstanceHostHeadless(AtlrInstance* restrict instance, const char* restrict name)
{
  atlrLog(LOG_INFO, "Initializing Antler instance in host headless mode ...");
  instance->mode = ATLR_MODE_HOST_HEADLESS;

  VkInstanceCreateInfo instanceInfo = {};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  
  VkApplicationInfo appInfo = {};
  if (!initAppInfo(&appInfo, name))
  {
    ATLR_LOG_ERROR("initAppInfo returned 0.");
    return 0;
  }
  instanceInfo.pApplicationInfo = &appInfo;

  // validation layer
#ifdef ATLR_DEBUG
  atlrLog(LOG_DEBUG, "Requiring Vulkan validation layer.");
  if (!isValidationLayerAvailable())
  {
    ATLR_LOG_ERROR("isValidationLayerAvailable returned 0.");
    return 0;
  }
  atlrLog(LOG_DEBUG, "Vulkan validation layer is available.");
  instanceInfo.enabledLayerCount = 1;
  instanceInfo.ppEnabledLayerNames = &validationLayer;

  VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
  initVkDebugUtilsMessengerCreateInfoEXT(&debugInfo);
  instanceInfo.pNext = &debugInfo;
#endif

  // extensions
  AtlrU32 extensionCount = 0;
  const char** extensions = malloc(sizeof(const char*));
#ifdef ATLR_DEBUG
  extensions[extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  extensionCount++;
#endif
  for (AtlrU32 i = 0; i < extensionCount; i++)
    atlrLog(LOG_DEBUG, "Requiring Vulkan instance extension \"%s\".", extensions[i]);
  if (!areInstanceExtensionsAvailable(extensions, extensionCount))
  {
    ATLR_LOG_ERROR("areInstanceExtensionsAvailable returned 0.");
    free(extensions);
    return 0;
  }
  instanceInfo.enabledExtensionCount = extensionCount;
  instanceInfo.ppEnabledExtensionNames = extensions; 

  // finish initializing instance
  if (vkCreateInstance(&instanceInfo, instance->allocator, &instance->instance) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("vkCreateInstance did not return VK_SUCCESS.");
    free(extensions);
    return 0;
  }
  free(extensions);
#ifdef ATLR_DEBUG
  if (!initDebugMessenger(instance, &debugInfo))
  {
    ATLR_LOG_ERROR("initDebugMessenger returned 0.");
    return 0;
  }
#endif
  instance->surface = VK_NULL_HANDLE;
  instance->data = NULL;

  atlrLog(LOG_INFO, "Done initializing Antler instance.");
  return 1;
}

void atlrDeinitInstanceHostHeadless(const AtlrInstance* restrict instance)
{
  if (instance->mode != ATLR_MODE_HOST_HEADLESS)
  {
    ATLR_LOG_ERROR("Antler is not in host headless mode.");
    return;
  }
  atlrLog(LOG_INFO, "Deinitializing antler instance in host headless mode ...");

#ifdef ATLR_DEBUG
  deinitDebugMessenger(instance);
#endif
  vkDestroyInstance(instance->instance, instance->allocator);
   
  atlrLog(LOG_INFO, "Done deinitializing Antler instance.");
}

AtlrU8 atlrInitInstanceHostGLFW(AtlrInstance* restrict instance, const int width, const int height, const char* restrict name)
{
  atlrLog(LOG_INFO, "Initializing Antler instance in GLFW mode ...");
  instance->mode = ATLR_MODE_HOST_GLFW;
  
  if (!glfwInit())
  {
    ATLR_LOG_ERROR("glfwInit returned GLFW_FALSE.");
    return 0;
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(width, height, name, NULL, NULL);
  if (!window)
  {
    ATLR_LOG_ERROR("glfwCreateWindow returned 0.");
    return 0;
  }
  
  VkInstanceCreateInfo instanceInfo = {};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  
  VkApplicationInfo appInfo = {};
  if (!initAppInfo(&appInfo, name))
  {
    ATLR_LOG_ERROR("initAppInfo returned 0.");
    return 0;
  }
  instanceInfo.pApplicationInfo = &appInfo;

  // validation layer
#ifdef ATLR_DEBUG
  atlrLog(LOG_DEBUG, "Requiring Vulkan validation layer.");
  if (!isValidationLayerAvailable())
  {
    ATLR_LOG_ERROR("isValidationLayerAvailable returned 0.");
    return 0;
  }
  atlrLog(LOG_DEBUG, "Vulkan validation layer is available.");
  instanceInfo.enabledLayerCount = 1;
  instanceInfo.ppEnabledLayerNames = &validationLayer;

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
    atlrLog(LOG_DEBUG, "Requiring Vulkan instance extension \"%s\".", extensions[i]);
  if (!areInstanceExtensionsAvailable(extensions, extensionCount))
  {
    ATLR_LOG_ERROR("areInstanceExtensionsAvailable returned 0.");
    free(extensions);
    return 0;
  }
  instanceInfo.enabledExtensionCount = extensionCount;
  instanceInfo.ppEnabledExtensionNames = extensions; 

  // finish initializing instance
  if (vkCreateInstance(&instanceInfo, instance->allocator, &instance->instance) != VK_SUCCESS)
  {
    ATLR_LOG_ERROR("vkCreateInstance did not return VK_SUCCESS.");
    free(extensions);
    return 0;
  }
  free(extensions);
#ifdef ATLR_DEBUG
  if (!initDebugMessenger(instance, &debugInfo))
  {
    ATLR_LOG_ERROR("initDebugMessenger returned 0.");
    return 0;
  }
#endif
  if (glfwCreateWindowSurface(instance->instance, window, instance->allocator, &instance->surface) != VK_SUCCESS)
    {
      ATLR_LOG_ERROR("glfwCreateWindowSurface did not return VK_SUCCESS.");
      return 0;
    }
  instance->data = window;

  atlrLog(LOG_INFO, "Done initializing Antler instance.");
  return 1;
}

void atlrDeinitInstanceHostGLFW(const AtlrInstance* restrict instance)
{
  if (instance->mode != ATLR_MODE_HOST_GLFW)
  {
    ATLR_LOG_ERROR("Antler is not in host GLFW mode.");
    return;
  }
  atlrLog(LOG_INFO, "Deinitializing Antler instance in host GLFW mode ...");

  vkDestroySurfaceKHR(instance->instance, instance->surface, instance->allocator);
#ifdef ATLR_DEBUG
  deinitDebugMessenger(instance);
#endif
  vkDestroyInstance(instance->instance, instance->allocator);

  GLFWwindow* window = instance->data;
  glfwDestroyWindow(window);
  glfwTerminate();
   
  atlrLog(LOG_INFO, "Done deinitializing Antler instance.");
}
