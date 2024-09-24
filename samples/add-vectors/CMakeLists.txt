set(ADD_VECTORS_SAMPLE_DIR "${SAMPLES_DIR}/add-vectors")
set(ADD_VECTORS_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/add-vectors")
add_executable(add-vectors-sample "${ADD_VECTORS_SAMPLE_DIR}/main.c")
target_link_libraries(add-vectors-sample PRIVATE antler)
compile_shader(
  "${ADD_VECTORS_SAMPLE_DIR}/add.comp.glsl"
  "${ADD_VECTORS_SAMPLE_BIN_DIR}/add-comp.spv")
add_custom_target(add-vectors-shaders ALL DEPENDS
  "${ADD_VECTORS_SAMPLE_BIN_DIR}/add-comp.spv")
