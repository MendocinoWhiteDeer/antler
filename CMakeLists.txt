cmake_minimum_required(VERSION 3.20)
project(antler C)
set(CMAKE_C_STANDARD 99)

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
  "src/camera.c")
target_include_directories(antler PUBLIC "${PROJECT_SOURCE_DIR}/src")

# GLFW
add_subdirectory(lib/glfw)
target_include_directories(antler PUBLIC "${PROJECT_SOURCE_DIR}/lib/glfw/include")
target_link_libraries(antler PUBLIC glfw)

# Vulkan
message("Requiring package \'Vulkan\'")
find_package(Vulkan REQUIRED)
target_include_directories(antler PUBLIC "${Vulkan_INCLUDE_DIRS}")
target_link_libraries(antler PUBLIC "${Vulkan_LIBRARIES}" glslang SPIRV glslang-default-resource-limits)

# shader compilation
function (compile_shader input_file output_file)
  add_custom_command(
    OUTPUT "${ARGV1}"
    COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" -V "${ARGV0}" -o "${ARGV1}"
    DEPENDS "${ARGV0}"
    COMMENT "Compiling glsl shader ${ARGV0} to ${ARGV1}")
endfunction()

# samples
set(SAMPLES_DIR "${PROJECT_SOURCE_DIR}/samples")
set(SAMPLES_BIN_DIR "${CMAKE_BINARY_DIR}/samples")
file(MAKE_DIRECTORY "${SAMPLES_BIN_DIR}")

# add-vectors sample
set(ADD_VECTORS_SAMPLE_DIR "${SAMPLES_DIR}/add-vectors")
set(ADD_VECTORS_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/add-vectors")
file(MAKE_DIRECTORY "${ADD_VECTORS_SAMPLE_BIN_DIR}")
add_executable(add-vectors-sample "${ADD_VECTORS_SAMPLE_DIR}/main.c")
set_target_properties(add-vectors-sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ADD_VECTORS_SAMPLE_BIN_DIR}")
target_link_libraries(add-vectors-sample PRIVATE antler)
compile_shader(
  "${ADD_VECTORS_SAMPLE_DIR}/add.comp.glsl"
  "${ADD_VECTORS_SAMPLE_BIN_DIR}/add-comp.spv")
add_custom_target(add-vectors-shaders ALL DEPENDS
  "${ADD_VECTORS_SAMPLE_BIN_DIR}/add-comp.spv")

# conway-game-of-life sample
set(CONWAY_LIFE_SAMPLE_DIR "${SAMPLES_DIR}/conway-game-of-life")
set(CONWAY_LIFE_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/conway-game-of-life")
file(MAKE_DIRECTORY "${CONWAY_LIFE_SAMPLE_BIN_DIR}")
add_executable(game-of-life-sample "${CONWAY_LIFE_SAMPLE_DIR}/main.c")
set_target_properties(game-of-life-sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CONWAY_LIFE_SAMPLE_BIN_DIR}")
target_link_libraries(game-of-life-sample PRIVATE antler)
compile_shader(
  "${CONWAY_LIFE_SAMPLE_DIR}/quad.vert.glsl"
  "${CONWAY_LIFE_SAMPLE_BIN_DIR}/quad-vert.spv")
compile_shader(
  "${CONWAY_LIFE_SAMPLE_DIR}/quad.frag.glsl"
  "${CONWAY_LIFE_SAMPLE_BIN_DIR}/quad-frag.spv")
add_custom_target(game-of-life-shaders ALL DEPENDS
  "${CONWAY_LIFE_SAMPLE_BIN_DIR}/quad-vert.spv"
  "${CONWAY_LIFE_SAMPLE_BIN_DIR}/quad-frag.spv")

#fragment-shader-client sample
set(FRAGMENT_SHADER_CLIENT_SAMPLE_DIR "${SAMPLES_DIR}/fragment-shader-client")
set(FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/fragment-shader-client")
file(MAKE_DIRECTORY "${FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR}")
add_executable(fragment-shader-client-sample "${FRAGMENT_SHADER_CLIENT_SAMPLE_DIR}/main.c")
set_target_properties(fragment-shader-client-sample PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR}")
target_link_libraries(fragment-shader-client-sample PRIVATE antler)
add_custom_command(				    
   OUTPUT "${FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR}/shaders"
   COMMAND cp -r "${SAMPLES_DIR}/fragment-shader-client/shaders" "${FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR}"
   DEPENDS "${SAMPLES_DIR}/fragment-shader-client/shaders"
   COMMENT "Copying glsl shaders for the fragment shader client sample.")
add_custom_target(fragment-shader-client-shaders ALL DEPENDS
   "${FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR}/shaders")

# hello-quad sample
set(HELLO_QUAD_SAMPLE_DIR "${SAMPLES_DIR}/hello-quad")
set(HELLO_QUAD_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/hello-quad")
file(MAKE_DIRECTORY "${HELLO_QUAD_SAMPLE_BIN_DIR}")
add_executable(hello-quad-sample "${HELLO_QUAD_SAMPLE_DIR}/main.c")
set_target_properties(hello-quad-sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${HELLO_QUAD_SAMPLE_BIN_DIR}")
target_link_libraries(hello-quad-sample PRIVATE antler)
compile_shader(
  "${HELLO_QUAD_SAMPLE_DIR}/quad.vert.glsl"
  "${HELLO_QUAD_SAMPLE_BIN_DIR}/quad-vert.spv")
compile_shader(
  "${HELLO_QUAD_SAMPLE_DIR}/quad.frag.glsl"
  "${HELLO_QUAD_SAMPLE_BIN_DIR}/quad-frag.spv")
add_custom_target(hello-quad-shaders ALL DEPENDS
  "${HELLO_QUAD_SAMPLE_BIN_DIR}/quad-vert.spv"
  "${HELLO_QUAD_SAMPLE_BIN_DIR}/quad-frag.spv")

# hello-triangle sample
set(HELLO_TRIANGLE_SAMPLE_DIR "${SAMPLES_DIR}/hello-triangle")
set(HELLO_TRIANGLE_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/hello-triangle")
file(MAKE_DIRECTORY "${HELLO_TRIANGLE_SAMPLE_BIN_DIR}")
add_executable(hello-triangle-sample "${HELLO_TRIANGLE_SAMPLE_DIR}/main.c")
set_target_properties(hello-triangle-sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${HELLO_TRIANGLE_SAMPLE_BIN_DIR}")
target_link_libraries(hello-triangle-sample PRIVATE antler)
compile_shader(
  "${HELLO_TRIANGLE_SAMPLE_DIR}/triangle.vert.glsl"
  "${HELLO_TRIANGLE_SAMPLE_BIN_DIR}/triangle-vert.spv")
compile_shader(
  "${HELLO_TRIANGLE_SAMPLE_DIR}/triangle.frag.glsl"
  "${HELLO_TRIANGLE_SAMPLE_BIN_DIR}/triangle-frag.spv")
add_custom_target(hello-triangle-shaders ALL DEPENDS
  "${HELLO_TRIANGLE_SAMPLE_BIN_DIR}/triangle-vert.spv"
  "${HELLO_TRIANGLE_SAMPLE_BIN_DIR}/triangle-frag.spv")

# rotating-cube sample
set(ROTATING_CUBE_SAMPLE_DIR "${SAMPLES_DIR}/rotating-cube")
set(ROTATING_CUBE_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/rotating-cube")
file(MAKE_DIRECTORY "${ROTATING_CUBE_SAMPLE_BIN_DIR}")
add_executable(rotating-cube-sample "${ROTATING_CUBE_SAMPLE_DIR}/main.c")
set_target_properties(rotating-cube-sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ROTATING_CUBE_SAMPLE_BIN_DIR}")
target_link_libraries(rotating-cube-sample PRIVATE antler)
compile_shader(
  "${ROTATING_CUBE_SAMPLE_DIR}/cube.vert.glsl"
  "${ROTATING_CUBE_SAMPLE_BIN_DIR}/cube-vert.spv")
compile_shader(
  "${ROTATING_CUBE_SAMPLE_DIR}/cube.frag.glsl"
  "${ROTATING_CUBE_SAMPLE_BIN_DIR}/cube-frag.spv")
add_custom_target(rotating-cube-shaders ALL DEPENDS
  "${ROTATING_CUBE_SAMPLE_BIN_DIR}/cube-vert.spv"
  "${ROTATING_CUBE_SAMPLE_BIN_DIR}/cube-frag.spv")
