cmake_minimum_required(VERSION 3.20)
project(antler C)
set(CMAKE_C_STANDARD 99)

message("Configuring antler ...")

find_package(Vulkan REQUIRED)

# default to release build if unspecified
if(NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE Release)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

# compile flags
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -DATLR_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3")

add_library(antler
  "src/logger.c"
  "src/util.c"
  "src/instance.c"
  "src/device.c"
  "src/commands.c"
  "src/buffer.c"
  "src/image.c"
  "src/pipeline.c"
  "src/render-pass.c"
  "src/swapchain.c")
target_include_directories(antler PUBLIC "${PROJECT_SOURCE_DIR}/src")

# GLFW
add_subdirectory(lib/glfw)
target_include_directories(antler PUBLIC "${PROJECT_SOURCE_DIR}/lib/glfw/include")
target_link_libraries(antler PUBLIC glfw)

# Vulkan
target_include_directories(antler PUBLIC "${Vulkan_INCLUDE_DIRS}")
target_link_libraries(antler PUBLIC "${Vulkan_LIBRARIES}")
