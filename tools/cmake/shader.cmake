# This source file is part of Epoxy licensed under the MIT License.
# See LICENSE.md file for details.

if(__shader)
  return()
endif()
set(__shader INCLUDED)

function(add_shader TARGET SHADER_FILE)
  get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WLE)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.metal
           ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.reflection.json
    DEPENDS shaders/${SHADER_FILE}
    COMMAND ${slang_sdk_SOURCE_DIR}/bin/slangc
            -line-directive-mode none
            -O0
            -o ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.metal
            -reflection-json ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.reflection.json
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_FILE}
  )

  xxd(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.metal)

endfunction()
