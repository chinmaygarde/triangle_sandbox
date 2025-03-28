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
    auto pipeline =
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
            .SetColorTargets({SDL_GPUColorTargetDescription{
                .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
                .blend_state = {},
            }})
            .Build(device);
    if (!pipeline.is_valid()) {
      return;
    }
    pipeline_ = std::move(pipeline);
  }

 private:
  UniqueGPUGraphicsPipeline pipeline_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Triangle);
};

}  // namespace ts
