#pragma once

#include <fml/closure.h>
#include <fml/mapping.h>
#include "buffer.h"
#include "drawable.h"
#include "macros.h"
#include "pipeline.h"
#include "shader.h"
#include "triangle_library.h"

namespace ts {

class Compute final : public Drawable {
 public:
  struct Vertex {
    glm::vec4 position;
    glm::vec2 uv;
  };
  static UniqueGPUGraphicsPipeline CreateRenderPipeline(
      const UniqueGPUDevice& device) {
    auto code = fml::NonOwnedMapping{xxd_triangle_library_data,
                                     xxd_triangle_library_length};
    auto vs = ShaderBuilder{}
                  .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                  .SetEntrypoint("SamplingVertexMain")
                  .SetStage(SDL_GPU_SHADERSTAGE_VERTEX)
                  .Build(device);

    auto fs = ShaderBuilder{}
                  .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                  .SetEntrypoint("SamplingFragmentMain")
                  .SetStage(SDL_GPU_SHADERSTAGE_FRAGMENT)
                  .SetResourceCounts(1, 0, 0, 0)
                  .Build(device);

    return GraphicsPipelineBuilder{}
        .SetVertexShader(&vs)
        .SetFragmentShader(&fs)
        .SetVertexAttribs({
            // Position
            SDL_GPUVertexAttribute{
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                .location = 0,
                .offset = offsetof(Vertex, position),
            },
            // UV
            SDL_GPUVertexAttribute{
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .location = 1,
                .offset = offsetof(Vertex, uv),
            },
        })
        .SetVertexBuffers({
            SDL_GPUVertexBufferDescription{
                .slot = 0,
                .pitch = sizeof(Vertex),
            },
        })
        .SetColorTargets({
            SDL_GPUColorTargetDescription{
                .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
            },
        })
        .SetPrimitiveType(SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP)
        .Build(device);
  }

  Compute(const UniqueGPUDevice& device) {
    auto code = fml::NonOwnedMapping{xxd_triangle_library_data,
                                     xxd_triangle_library_length};
    compute_pipeline_ = ComputePipelineBuilder{}
                            .SetDimensions({1, 1, 1})
                            .SetShader(&code, SDL_GPU_SHADERFORMAT_MSL)
                            .SetEntrypoint("ComputeAdder")
                            .SetResourceCounts(0, 0, 0, 1, 0, 0)
                            .Build(device);
    if (!compute_pipeline_.is_valid()) {
      return;
    }

    render_pipeline_ = CreateRenderPipeline(device);
    if (!render_pipeline_.is_valid()) {
      return;
    }

    render_vtx_buffer_ =
        PerformHostToDeviceTransfer(device,
                                    std::vector<Vertex>{
                                        // 0
                                        Vertex{
                                            .position = {-1, -1, 0, 1},
                                            .uv = {0, 1},
                                        },
                                        // 1
                                        Vertex{
                                            .position = {-1, 1, 0, 1},
                                            .uv = {0, 0},
                                        },
                                        // 2
                                        Vertex{
                                            .position = {1, -1, 0, 1},
                                            .uv = {1, 1},
                                        },
                                        // 3
                                        Vertex{
                                            .position = {1, 1, 0, 1},
                                            .uv = {1, 0},
                                        },
                                    },
                                    SDL_GPU_BUFFERUSAGE_VERTEX);
    if (!render_vtx_buffer_.is_valid()) {
      return;
    }

    render_sampler_ = CreateSampler(
        device.get(),
        SDL_GPUSamplerCreateInfo{
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
        });
    if (!render_sampler_.is_valid()) {
      return;
    }

    glm::ivec2 dims = {800, 600};
    texture_ = CreateGPUTexture(device.get(), {dims, 1}, SDL_GPU_TEXTURETYPE_2D,
                                SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE |
                                    SDL_GPU_TEXTUREUSAGE_SAMPLER);
    if (!texture_.is_valid()) {
      return;
    }

    auto command_buffer = SDL_AcquireGPUCommandBuffer(device.get());
    if (!command_buffer) {
      return;
    }
    FML_DEFER(SDL_SubmitGPUCommandBuffer(command_buffer));

    SDL_GPUStorageTextureReadWriteBinding binding = {
        .texture = texture_.get().value,
    };
    auto compute_pass =
        SDL_BeginGPUComputePass(command_buffer, &binding, 1u, nullptr, 0u);
    if (!compute_pass) {
      return;
    }
    FML_DEFER(SDL_EndGPUComputePass(compute_pass));

    SDL_BindGPUComputePipeline(compute_pass, compute_pipeline_.get().value);
    SDL_DispatchGPUCompute(compute_pass, 800, 600, 1);

    is_valid_ = true;
  }

  bool Draw(SDL_GPURenderPass* pass) override {
    if (!is_valid_) {
      return false;
    }

    SDL_BindGPUGraphicsPipeline(pass, render_pipeline_.get().value);
    SDL_GPUBufferBinding vtx_binding = {
        .buffer = render_vtx_buffer_.get().value,
    };
    SDL_BindGPUVertexBuffers(pass, 0, &vtx_binding, 1u);
    SDL_GPUTextureSamplerBinding frag_sampler_bindings = {
        .sampler = render_sampler_.get().value,
        .texture = texture_.get().value,
    };
    SDL_BindGPUFragmentSamplers(pass, 0, &frag_sampler_bindings, 1u);
    SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);

    return true;
  }

 private:
  UniqueGPUComputePipeline compute_pipeline_;
  UniqueGPUGraphicsPipeline render_pipeline_;
  UniqueGPUTexture texture_;
  UniqueGPUBuffer render_vtx_buffer_;
  UniqueGPUSampler render_sampler_;
  bool is_valid_ = false;
  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Compute);
};

}  // namespace ts
