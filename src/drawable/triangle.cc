#include "triangle.h"

#include <glm/glm.hpp>
#include "buffer.h"
#include "graphics_pipeline.h"
#include "macros.h"
#include "shader.h"
#include "triangle.slang.h"

namespace ts {

Triangle::Triangle(const Context& ctx) {
  auto code = fml::NonOwnedMapping(xxd_triangle_data, xxd_triangle_length);

  auto vs = ShaderBuilder{}
                .SetStage(SDL_GPU_SHADERSTAGE_VERTEX)
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetEntrypoint("VertexMain")
                .Build(ctx.GetDevice());
  auto fs = ShaderBuilder{}
                .SetStage(SDL_GPU_SHADERSTAGE_FRAGMENT)
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetEntrypoint("FragmentMain")
                .Build(ctx.GetDevice());
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
              .format = ctx.GetColorFormat(),
          }})
          .SetPrimitiveType(SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP)
          .SetSampleCount(ctx.GetColorSamples())
          .SetDepthStencilFormat(ctx.GetDepthFormat())
          .Build(ctx.GetDevice());
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
  auto vtx_buffer = PerformHostToDeviceTransfer(ctx.GetDevice(), vertices,
                                                SDL_GPU_BUFFERUSAGE_VERTEX);
  if (!vtx_buffer.is_valid()) {
    return;
  }
  vtx_buffer_ = std::move(vtx_buffer);
  is_valid_ = true;
}

bool Triangle::Draw(const DrawContext& context) {
  if (!is_valid_) {
    return false;
  }
  SDL_PushGPUDebugGroup(context.command_buffer, "Triangle");
  FML_DEFER(SDL_PopGPUDebugGroup(context.command_buffer));

  SDL_BindGPUGraphicsPipeline(context.pass, pipeline_.get().value);
  {
    SDL_GPUBufferBinding binding = {
        .buffer = vtx_buffer_.get().value,
        .offset = 0u,
    };
    SDL_BindGPUVertexBuffers(context.pass, 0u, &binding, 1u);
  }
  SDL_DrawGPUPrimitives(context.pass, 3, 1, 0, 0);
  return true;
}

}  // namespace ts
