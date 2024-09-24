set(GOOCH_SHADING_SAMPLE_DIR "${SAMPLES_DIR}/gooch-shading")
set(GOOCH_SHADING_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/gooch-shading")
add_executable(gooch-shading-sample "${GOOCH_SHADING_SAMPLE_DIR}/main.c")
target_link_libraries(gooch-shading-sample PRIVATE antler)
compile_shader(
  "${GOOCH_SHADING_SAMPLE_DIR}/gooch.vert.glsl"
  "${GOOCH_SHADING_SAMPLE_BIN_DIR}/gooch-vert.spv")
compile_shader(
  "${GOOCH_SHADING_SAMPLE_DIR}/gooch.frag.glsl"
  "${GOOCH_SHADING_SAMPLE_BIN_DIR}/gooch-frag.spv")
compile_shader(	
  "${GOOCH_SHADING_SAMPLE_DIR}/edge.vert.glsl"
  "${GOOCH_SHADING_SAMPLE_BIN_DIR}/edge-vert.spv")
compile_shader(	
  "${GOOCH_SHADING_SAMPLE_DIR}/edge.frag.glsl"
  "${GOOCH_SHADING_SAMPLE_BIN_DIR}/edge-frag.spv")
add_custom_target(gooch-shading-shaders ALL DEPENDS
  "${GOOCH_SHADING_SAMPLE_BIN_DIR}/gooch-vert.spv"
  "${GOOCH_SHADING_SAMPLE_BIN_DIR}/gooch-frag.spv"
  "${GOOCH_SHADING_SAMPLE_BIN_DIR}/edge-vert.spv"
  "${GOOCH_SHADING_SAMPLE_BIN_DIR}/edge-frag.spv")
