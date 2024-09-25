if (ATLR_BUILD_HOST_GLFW)
  set(CONWAY_LIFE_SAMPLE_DIR "${SAMPLES_DIR}/conway-game-of-life")
  set(CONWAY_LIFE_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/conway-game-of-life")
  add_executable(game-of-life-sample "${CONWAY_LIFE_SAMPLE_DIR}/main.c")
  target_link_libraries(game-of-life-sample PRIVATE antler-host-glfw) 
  compile_shader(
	"${CONWAY_LIFE_SAMPLE_DIR}/quad.vert.glsl"
  	"${CONWAY_LIFE_SAMPLE_BIN_DIR}/quad-vert.spv")
  compile_shader(
	"${CONWAY_LIFE_SAMPLE_DIR}/quad.frag.glsl"
  	"${CONWAY_LIFE_SAMPLE_BIN_DIR}/quad-frag.spv")
  add_custom_target(game-of-life-shaders ALL DEPENDS
  	"${CONWAY_LIFE_SAMPLE_BIN_DIR}/quad-vert.spv"
  	"${CONWAY_LIFE_SAMPLE_BIN_DIR}/quad-frag.spv")
endif()
