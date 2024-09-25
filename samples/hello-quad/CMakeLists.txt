if (ATLR_BUILD_HOST_GLFW)
  set(HELLO_QUAD_SAMPLE_DIR "${SAMPLES_DIR}/hello-quad")
  set(HELLO_QUAD_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/hello-quad")
  add_executable(hello-quad-sample "${HELLO_QUAD_SAMPLE_DIR}/main.c")
  target_link_libraries(hello-quad-sample PRIVATE antler-host-glfw)
  compile_shader(
	"${HELLO_QUAD_SAMPLE_DIR}/quad.vert.glsl"
  	"${HELLO_QUAD_SAMPLE_BIN_DIR}/quad-vert.spv")
  compile_shader(
	"${HELLO_QUAD_SAMPLE_DIR}/quad.frag.glsl"
   	"${HELLO_QUAD_SAMPLE_BIN_DIR}/quad-frag.spv")
  add_custom_target(hello-quad-shaders ALL DEPENDS
  	"${HELLO_QUAD_SAMPLE_BIN_DIR}/quad-vert.spv"
  	"${HELLO_QUAD_SAMPLE_BIN_DIR}/quad-frag.spv")
endif()
