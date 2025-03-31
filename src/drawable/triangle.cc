#include "triangle.h"

#include <glm/glm.hpp>
#include "buffer.h"
#include "pipeline.h"
#include "shader.h"
#include "triangle.slang.h"

namespace ts {

Triangle::Triangle(const UniqueGPUDevice& device) {
  auto code = fml::NonOwnedMapping(xxd_triangle_data, xxd_triangle_length);

  auto vs = ShaderBuilder{}
                .SetStage(SDL_GPU_SHADERSTAGE_VERTEX)
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetEntrypoint("VertexMain")
                .Build(device);
  auto fs = ShaderBuilder{}
                .SetStage(SDL_GPU_SHADERSTAGE_FRAGMENT)
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetEntrypoint("FragmentMain")
                .Build(device);
  struct Vertex {
    glm::vec4 position = {};
    glm::vec4 color = {};
  };
  auto pipeline =
      GraphicsPipelineBuilder{}
          .SetVertexShader(&vs)
          .SetFragmentShader(&fs)
          .SetPrimitiveType(SDL_GPU_PRIMITIVETYPE_TRIANGLELIST)
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
          .SetPrimitiveType(SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP)
          .Build(device);
  if (!pipeline.is_valid()) {
    return;
  }
  pipeline_ = std::move(pipeline);
  std::vector<Vertex> vertices = {
      Vertex{
          .color = glm::vec4{1.0, 0.0, 0.0, 1.0},
          .position = glm::vec4{0.0, 0.5, 0.0, 1.0},
      },
      Vertex{
          .color = glm::vec4{0.0, 1.0, 0.0, 1.0},
          .position = glm::vec4{0.5, -0.5, 0.0, 1.0},
      },
      Vertex{
          .color = glm::vec4{0.0, 0.0, 1.0, 1.0},
          .position = glm::vec4{-0.5, -0.5, 0.0, 1.0},
      },
  };
  auto vtx_buffer =
      PerformHostToDeviceTransfer(device, vertices, SDL_GPU_BUFFERUSAGE_VERTEX);
  if (!vtx_buffer.is_valid()) {
    return;
  }
  vtx_buffer_ = std::move(vtx_buffer);
  is_valid_ = true;
}

bool Triangle::Draw(SDL_GPURenderPass* pass) {
  if (!is_valid_) {
    return false;
  }
  SDL_BindGPUGraphicsPipeline(pass, pipeline_.get().value);
  {
    SDL_GPUBufferBinding binding = {
        .buffer = vtx_buffer_.get().value,
        .offset = 0u,
    };
    SDL_BindGPUVertexBuffers(pass, 0u, &binding, 1u);
  }
  SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
  return true;
}

}  // namespace ts
