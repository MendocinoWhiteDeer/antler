set(FRAGMENT_SHADER_CLIENT_SAMPLE_DIR "${SAMPLES_DIR}/fragment-shader-client")
set(FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR "${SAMPLES_BIN_DIR}/fragment-shader-client")
add_executable(fragment-shader-client-sample "${FRAGMENT_SHADER_CLIENT_SAMPLE_DIR}/main.c")
target_link_libraries(fragment-shader-client-sample PRIVATE antler)
add_custom_command(				    
   OUTPUT "${FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR}/shaders"
   COMMAND cp -r "${SAMPLES_DIR}/fragment-shader-client/shaders" "${FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR}"
   DEPENDS "${SAMPLES_DIR}/fragment-shader-client/shaders"
   COMMENT "Copying glsl shaders for the fragment shader client sample.")
add_custom_target(fragment-shader-client-shaders ALL DEPENDS
   "${FRAGMENT_SHADER_CLIENT_SAMPLE_BIN_DIR}/shaders")
