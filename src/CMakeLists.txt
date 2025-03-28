# Library
add_executable(triangle_sandbox
  context.cc
  context.h
  main.cc
  pipeline.cc
  pipeline.cc
  renderer.cc
  renderer.h
  sdl_types.h
  shader.cc
  shader.h
  triangle.cc
  triangle.h
)

target_include_directories(triangle_sandbox
  PUBLIC
    ../third_party/hedley
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/triangle.slang.metal
  DEPENDS shaders/triangle.slang
  COMMAND ${slang_sdk_SOURCE_DIR}/bin/slangc
          -g
          -target metal
          -o ${CMAKE_CURRENT_BINARY_DIR}/triangle.slang.metal
          ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.slang
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/triangle_library.h
         ${CMAKE_CURRENT_BINARY_DIR}/triangle_library.cc
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/triangle.slang.metal
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/xxd.py
            --symbol-name=triangle_library
            --output-header=${CMAKE_CURRENT_BINARY_DIR}/triangle_library.h
            --output-source=${CMAKE_CURRENT_BINARY_DIR}/triangle_library.cc
            --source=${CMAKE_CURRENT_BINARY_DIR}/triangle.slang.metal
)

target_sources(triangle_sandbox
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/triangle_library.h
    ${CMAKE_CURRENT_BINARY_DIR}/triangle_library.cc
)

target_link_libraries(triangle_sandbox
  PUBLIC
    SDL3-static
    jfml
    glm
)
