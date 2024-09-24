set(HELLO_TRIANGLE_SAMPLE_DIR "${SAMPLES_DIR}/hello-triangle")
set(HELLO_TRIANGLE_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/hello-triangle")
add_executable(hello-triangle-sample "${HELLO_TRIANGLE_SAMPLE_DIR}/main.c")
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
