cmake_minimum_required(VERSION 3.20)
project(antler LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

message("Configuring antler ...")

#find_package(glslang REQUIRED)

# default to release build if unspecified
if(NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE Release)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

# define the antler build modes (glfw, headless, and hook)
option(ATLR_BUILD_HOST_HEADLESS "Build antler in headless host mode" ON)
option(ATLR_BUILD_HOST_GLFW "Build antler in GLFW host mode" ON)
option(ATLR_BUILD_HOST_GLFW_IMGUI "Build antler in GLFW host mode with imgui" ON)
option(ATLR_BUILD_HOOK "Build antler in hook mode" ON)
message("Build antler in headless host mode: ${ATLR_BUILD_HOST_HEADLESS}")
message("Build antler in GLFW host mode: ${ATLR_BUILD_HOST_GLFW}")
message("Build antler in GLFW host mode with imgui: ${ATLR_BUILD_HOST_GLFW_IMGUI}")
message("Build antler in hook mode: ${ATLR_BUILD_HOOK}")

# compile flags
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -DATLR_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -DATLR_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# antler libraries ###################################################################################
####################################################################################################

if (ATLR_BUILD_HOST_HEADLESS)
   add_library(antler-host-headless	
  	"src/transforms.c"
  	"src/logger.c"
	"src/util.c"
	"src/instance.c"
	"src/device.c"
	"src/commands.c"
	"src/buffer.c"
	"src/image.c"
	"src/descriptor.c"
	"src/pipeline.c"
	"src/render-pass.c"
	"src/offscreen-canvas.c")
  target_include_directories(antler-host-headless PUBLIC "${PROJECT_SOURCE_DIR}/src")
  target_compile_definitions(antler-host-headless PUBLIC ATLR_BUILD_HOST_HEADLESS)
endif()

if (ATLR_BUILD_HOST_GLFW)
  add_library(antler-host-glfw	
  	"src/transforms.c"
  	"src/logger.c"
	"src/util.c"
	"src/instance.c"
	"src/device.c"
	"src/commands.c"
	"src/buffer.c"
	"src/image.c"
	"src/descriptor.c"
	"src/pipeline.c"
	"src/render-pass.c"
	"src/swapchain.c"
	"src/offscreen-canvas.c"
	"src/camera.c")
  target_include_directories(antler-host-glfw PUBLIC "${PROJECT_SOURCE_DIR}/src")
  target_compile_definitions(antler-host-glfw PUBLIC ATLR_BUILD_HOST_GLFW)
endif()

if (ATLR_BUILD_HOST_GLFW_IMGUI AND NOT ATLR_BUILD_HOST_GLFW)
  add_library(antler-host-glfw OBJECT
  	"src/transforms.c"
  	"src/logger.c"
	"src/util.c"
	"src/instance.c"
	"src/device.c"
	"src/commands.c"
	"src/buffer.c"
	"src/image.c"
	"src/descriptor.c"
	"src/pipeline.c"
	"src/render-pass.c"
	"src/swapchain.c"
	"src/offscreen-canvas.c"
	"src/camera.c")
  target_include_directories(antler-host-glfw PUBLIC "${PROJECT_SOURCE_DIR}/src")
  target_compile_definitions(antler-host-glfw PUBLIC ATLR_BUILD_HOST_GLFW)
endif()

if (ATLR_BUILD_HOOK)
   add_library(antler-hook	
  	"src/transforms.c"
  	"src/logger.c"
	"src/util.c"
	"src/instance.c"
	"src/device.c"
	"src/commands.c"
	"src/buffer.c"
	"src/image.c"
	"src/descriptor.c"
	"src/pipeline.c"
	"src/render-pass.c"
	"src/offscreen-canvas.c")
  target_include_directories(antler-hook PUBLIC "${PROJECT_SOURCE_DIR}/src")
  target_compile_definitions(antler-hook PUBLIC ATLR_BUILD_HOOK)
endif()

# GLFW
if (ATLR_BUILD_HOST_GLFW)
  add_subdirectory(lib/glfw)	
  target_include_directories(antler-host-glfw PUBLIC "${PROJECT_SOURCE_DIR}/lib/glfw/include")
  target_link_libraries(antler-host-glfw PUBLIC glfw)
endif()

# stb
if (ATLR_BUILD_HOST_HEADLESS)
 target_include_directories(antler-host-headless PUBLIC "${PROJECT_SOURCE_DIR}/lib/stb")
endif()
if (ATLR_BUILD_HOST_GLFW)
 target_include_directories(antler-host-glfw PUBLIC "${PROJECT_SOURCE_DIR}/lib/stb")
endif()
if (ATLR_BUILD_HOOK)
 target_include_directories(antler-hook PUBLIC "${PROJECT_SOURCE_DIR}/lib/stb")
endif()

# Vulkan
message("Requiring package \'Vulkan\'")
find_package(Vulkan REQUIRED)
if (ATLR_BUILD_HOST_HEADLESS)
 target_include_directories(antler-host-headless PUBLIC "${Vulkan_INCLUDE_DIRS}")
 target_link_libraries(antler-host-headless PUBLIC
 	"${Vulkan_LIBRARIES}" glslang SPIRV glslang-default-resource-limits)
endif()
if (ATLR_BUILD_HOST_GLFW)
 target_include_directories(antler-host-glfw PUBLIC "${Vulkan_INCLUDE_DIRS}")
 target_link_libraries(antler-host-glfw PUBLIC
 	"${Vulkan_LIBRARIES}" glslang SPIRV glslang-default-resource-limits)
endif()
if (ATLR_BUILD_HOOK)
 target_include_directories(antler-hook PUBLIC "${Vulkan_INCLUDE_DIRS}")
 target_link_libraries(antler-hook PUBLIC
 	"${Vulkan_LIBRARIES}" glslang SPIRV glslang-default-resource-limits)
endif()

# antler imgui lib
if (ATLR_BUILD_HOST_GLFW_IMGUI)
  add_library(antler-imgui
	"src/antler-imgui.cpp"
	"lib/imgui/imgui.cpp"
	"lib/imgui/imgui_draw.cpp"
	"lib/imgui/imgui_tables.cpp"
	"lib/imgui/imgui_widgets.cpp"
	"lib/imgui/backends/imgui_impl_glfw.cpp")
  target_include_directories(antler-imgui PUBLIC
	"${PROJECT_SOURCE_DIR}/lib/imgui"
	"${PROJECT_SOURCE_DIR}/lib/imgui/backends")
  target_link_libraries(antler-imgui PUBLIC antler-host-glfw)
endif()

# samples
add_subdirectory(samples)
