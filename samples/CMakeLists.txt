# shader compilation
function (compile_shader input_file output_file)
  add_custom_command(
    OUTPUT "${ARGV1}"
    COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" -V "${ARGV0}" -o "${ARGV1}"
    DEPENDS "${ARGV0}"
    COMMENT "Compiling glsl shader ${ARGV0} to ${ARGV1}")
endfunction()

set(SAMPLES_DIR "${PROJECT_SOURCE_DIR}/samples")
set(SAMPLES_BIN_DIR "${CMAKE_BINARY_DIR}/samples")
file(MAKE_DIRECTORY "${SAMPLES_BIN_DIR}")

add_subdirectory(add-vectors)
add_subdirectory(conway-game-of-life)
add_subdirectory(fragment-shader-client)
add_subdirectory(gooch-shading)
add_subdirectory(hello-quad)
add_subdirectory(hello-triangle)
add_subdirectory(rotating-cube)
add_subdirectory(shell-texturing)
add_subdirectory(transform-cube)
