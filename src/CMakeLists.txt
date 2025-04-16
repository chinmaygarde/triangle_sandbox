# Library
add_executable(triangle_sandbox
  buffer.cc
  buffer.h
  context.cc
  context.h
  drawable.cc
  drawable.h
  drawable/compute.cc
  drawable/compute.h
  drawable/model_renderer.cc
  drawable/model_renderer.h
  drawable/triangle.cc
  drawable/triangle.h
  macros.h
  main.cc
  model.cc
  model.h
  pipeline.cc
  pipeline.h
  renderer.cc
  renderer.h
  sdl_types.cc
  sdl_types.h
  shader.cc
  shader.h
  ../third_party/imgui/backends/imgui_impl_sdl3.cpp
  ../third_party/imgui/backends/imgui_impl_sdl3.h
  ../third_party/imgui/backends/imgui_impl_sdlgpu3.cpp
  ../third_party/imgui/backends/imgui_impl_sdlgpu3.h
  ../third_party/imgui/imgui.cpp
  ../third_party/imgui/imgui.h
  ../third_party/imgui/imgui_draw.cpp
  ../third_party/imgui/imgui_tables.cpp
  ../third_party/imgui/imgui_widgets.cpp
  ../third_party/imgui/imgui_demo.cpp
)

target_include_directories(triangle_sandbox
  PUBLIC
    .
    ../third_party/hedley
    ../third_party/imgui
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(triangle_sandbox
  PUBLIC
    -DGLM_FORCE_LEFT_HANDED=1
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE=1
)

get_filename_component(MODELS_DIRECTORY ../third_party/gltf_sample_assets/Models ABSOLUTE)
set(MODELS_LOCATION ${MODELS_DIRECTORY})
configure_file(models_location.h.in models_location.h @ONLY)
target_sources(triangle_sandbox PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/models_location.h)

add_shader(triangle_sandbox triangle.slang)
add_shader(triangle_sandbox sampling.slang)
add_shader(triangle_sandbox compute.slang)
add_shader(triangle_sandbox model.slang)

target_link_libraries(triangle_sandbox
  PUBLIC
    SDL3-static
    jfml
    glm
    tinygltf
)
