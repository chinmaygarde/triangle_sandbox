#pragma once

#include <SDL3/SDL_gpu.h>
#include <fml/logging.h>
#include <fml/mapping.h>
#include <glm/glm.hpp>
#include "pipeline.h"
#include "sdl_types.h"
#include "shader.h"
#include "triangle_library.h"

namespace ts {

class Triangle {
 public:
  Triangle(const UniqueGPUDevice& device) {
    auto code = fml::NonOwnedMapping(xxd_triangle_library_data,
                                     xxd_triangle_library_length);

    auto vs = ShaderBuilder{}
                  .SetStage(ShaderStage::kVertex)
                  .SetCode(&code, ShaderFormat::kMSL)
                  .SetEntrypoint("VertexMain")
                  .Build(device);
    auto fs = ShaderBuilder{}
                  .SetStage(ShaderStage::kFragment)
                  .SetCode(&code, ShaderFormat::kMSL)
                  .SetEntrypoint("FragmentMain")
                  .Build(device);
    struct Vertex {
      glm::vec4 position;
      glm::vec4 color;
    };
    auto pipe =
        GraphicsPipelineBuilder{}
            .SetVertexShader(&vs)
            .SetFragmentShader(&fs)
            .SetPrimitiveType(PrimitiveType::kTriangleList)
            .SetVertexAttribs({SDL_GPUVertexAttribute{
                                   // Position
                                   .location = 0,
                                   .buffer_slot = 0,
                                   .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                                   .offset = offsetof(Vertex, position),
                               },
                               SDL_GPUVertexAttribute{
                                   // Color
                                   .location = 1,
                                   .buffer_slot = 0,
                                   .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                                   .offset = offsetof(Vertex, color),
                               }})
            .SetVertexBuffers({SDL_GPUVertexBufferDescription{
                .slot = 0u,
                .pitch = sizeof(Vertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            }})
            .Build(device);
  }

 private:
  UniqueGPUShader shader_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Triangle);
};

}  // namespace ts
