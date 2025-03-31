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
    DEPENDS shaders/${SHADER_FILE}
    COMMAND ${slang_sdk_SOURCE_DIR}/bin/slangc
            -g
            -target metal
            -o ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.metal
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_FILE}
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.h
           ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.cc
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.metal
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/xxd.py
              --symbol-name=${SHADER_NAME}
              --output-header=${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.h
              --output-source=${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.cc
              --source=${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.metal
  )

  target_sources(${TARGET}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.h
      ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}.cc
  )

endfunction()
