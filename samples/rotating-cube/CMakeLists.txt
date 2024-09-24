set(ROTATING_CUBE_SAMPLE_DIR "${SAMPLES_DIR}/rotating-cube")
set(ROTATING_CUBE_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/rotating-cube")
add_executable(rotating-cube-sample "${ROTATING_CUBE_SAMPLE_DIR}/main.c")
target_link_libraries(rotating-cube-sample PRIVATE antler)
compile_shader(
  "${ROTATING_CUBE_SAMPLE_DIR}/diffuse.vert.glsl"
  "${ROTATING_CUBE_SAMPLE_BIN_DIR}/diffuse-vert.spv")
compile_shader(
  "${ROTATING_CUBE_SAMPLE_DIR}/diffuse.frag.glsl"
  "${ROTATING_CUBE_SAMPLE_BIN_DIR}/diffuse-frag.spv")
add_custom_target(rotating-cube-shaders ALL DEPENDS
  "${ROTATING_CUBE_SAMPLE_BIN_DIR}/diffuse-vert.spv"
  "${ROTATING_CUBE_SAMPLE_BIN_DIR}/diffuse-frag.spv")
