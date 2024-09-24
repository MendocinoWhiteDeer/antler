set(TRANSFORM_CUBE_SAMPLE_DIR "${SAMPLES_DIR}/transform-cube")
set(TRANSFORM_CUBE_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/transform-cube")
add_executable(transform-cube-sample "${TRANSFORM_CUBE_SAMPLE_DIR}/main.cpp")
target_link_libraries(transform-cube-sample PRIVATE antler-imgui)
compile_shader(
  "${TRANSFORM_CUBE_SAMPLE_DIR}/diffuse.vert.glsl"
  "${TRANSFORM_CUBE_SAMPLE_BIN_DIR}/diffuse-vert.spv")
compile_shader(
  "${TRANSFORM_CUBE_SAMPLE_DIR}/diffuse.frag.glsl"
  "${TRANSFORM_CUBE_SAMPLE_BIN_DIR}/diffuse-frag.spv")
add_custom_target(transform-shaders ALL DEPENDS
  "${TRANSFORM_CUBE_SAMPLE_BIN_DIR}/diffuse-vert.spv"
  "${TRANSFORM_CUBE_SAMPLE_BIN_DIR}/diffuse-frag.spv")
