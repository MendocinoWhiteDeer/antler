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

# compile flags
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -DATLR_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# antler library ###################################################################################
####################################################################################################

add_library(antler	
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
target_include_directories(antler PUBLIC "${PROJECT_SOURCE_DIR}/src")

# GLFW
add_subdirectory(lib/glfw)
target_include_directories(antler PUBLIC "${PROJECT_SOURCE_DIR}/lib/glfw/include")
target_link_libraries(antler PUBLIC glfw)

# stb
target_include_directories(antler PUBLIC "${PROJECT_SOURCE_DIR}/lib/stb")

# Vulkan
message("Requiring package \'Vulkan\'")
find_package(Vulkan REQUIRED)
target_include_directories(antler PUBLIC "${Vulkan_INCLUDE_DIRS}")
target_link_libraries(antler PUBLIC "${Vulkan_LIBRARIES}" glslang SPIRV glslang-default-resource-limits)

# antler imgui library

add_library(antler-imgui
	"src/antler-imgui.cpp"
	"lib/imgui/imgui.cpp"
	"lib/imgui/imgui_draw.cpp"
	"lib/imgui/imgui_tables.cpp"
	"lib/imgui/imgui_widgets.cpp"
	"lib/imgui/backends/imgui_impl_glfw.cpp")
target_include_directories(antler-imgui PUBLIC "${PROJECT_SOURCE_DIR}/lib/imgui" "${PROJECT_SOURCE_DIR}/lib/imgui/backends")
target_link_libraries(antler-imgui PUBLIC antler)

# samples
add_subdirectory(samples)
