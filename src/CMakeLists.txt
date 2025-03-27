# Library
add_executable(triangle_sandbox
  context.cc
  context.h
  main.cc
  renderer.cc
  renderer.h
  sdl_types.h
)

target_include_directories(triangle_sandbox PUBLIC ../third_party/hedley)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/triangle.slang.metal
  DEPENDS shaders/triangle.slang
  COMMAND slangc -g -target metal triangle.slang
)

target_link_libraries(triangle_sandbox PUBLIC SDL3-static jfml)
