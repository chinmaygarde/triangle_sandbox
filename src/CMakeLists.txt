# Library
add_executable(triangle_sandbox
  buffer.cc
  buffer.h
  context.cc
  context.h
  drawable.cc
  drawable.h
  main.cc
  pipeline.cc
  pipeline.h
  renderer.cc
  renderer.h
  sdl_types.h
  sdl_types.cc
  shader.cc
  shader.h
  macros.h
  drawable/triangle.cc
  drawable/compute.cc
  drawable/triangle.h
  drawable/compute.h
)

target_include_directories(triangle_sandbox
  PUBLIC
    .
    ../third_party/hedley
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_shader(triangle_sandbox triangle.slang)
add_shader(triangle_sandbox sampling.slang)
add_shader(triangle_sandbox compute.slang)

target_link_libraries(triangle_sandbox
  PUBLIC
    SDL3-static
    jfml
    glm
)
