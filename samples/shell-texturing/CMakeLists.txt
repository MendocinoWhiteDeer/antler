if (ATLR_BUILD_HOST_GLFW)
  set(SHELL_TEXTURING_SAMPLE_DIR "${SAMPLES_DIR}/shell-texturing")
  set(SHELL_TEXTURING_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/shell-texturing")
  add_executable(shell-texturing-sample "${SHELL_TEXTURING_SAMPLE_DIR}/main.cpp")
  target_link_libraries(shell-texturing-sample PRIVATE antler-imgui)
  compile_shader(
	"${SHELL_TEXTURING_SAMPLE_DIR}/shell.vert.glsl"
  	"${SHELL_TEXTURING_SAMPLE_BIN_DIR}/shell-vert.spv")
  compile_shader(
	"${SHELL_TEXTURING_SAMPLE_DIR}/shell.geom.glsl"
  	"${SHELL_TEXTURING_SAMPLE_BIN_DIR}/shell-geom.spv")
  compile_shader(
	"${SHELL_TEXTURING_SAMPLE_DIR}/grass.frag.glsl"
  	"${SHELL_TEXTURING_SAMPLE_BIN_DIR}/grass-frag.spv")
  add_custom_target(shell-texturing-shaders ALL DEPENDS
  	"${SHELL_TEXTURING_SAMPLE_BIN_DIR}/shell-vert.spv"
  	"${SHELL_TEXTURING_SAMPLE_BIN_DIR}/shell-geom.spv"
  	"${SHELL_TEXTURING_SAMPLE_BIN_DIR}/grass-frag.spv")
endif()
