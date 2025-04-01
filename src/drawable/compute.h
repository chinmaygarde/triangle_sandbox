#pragma once

#include <fml/closure.h>
#include <fml/mapping.h>
#include "buffer.h"
#include "compute.slang.h"
#include "drawable.h"
#include "macros.h"
#include "pipeline.h"
#include "sampling.slang.h"
#include "shader.h"

namespace ts {

class Compute final : public Drawable {
 public:
  struct Vertex {
    glm::vec4 position;
    glm::vec2 uv;
  };
  static UniqueGPUGraphicsPipeline CreateRenderPipeline(
      const UniqueGPUDevice& device) {
    auto code = fml::NonOwnedMapping{xxd_sampling_data, xxd_sampling_length};
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
    auto code = fml::NonOwnedMapping{xxd_compute_data, xxd_compute_length};
    compute_pipeline_ =
        ComputePipelineBuilder{}
            .SetDimensions({thread_count_.x, thread_count_.y, 1})
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
            .min_filter = SDL_GPU_FILTER_NEAREST,
            .mag_filter = SDL_GPU_FILTER_NEAREST,
        });
    if (!render_sampler_.is_valid()) {
      return;
    }

    rw_texture_ =
        CreateGPUTexture(device.get(), {800, 600, 1}, SDL_GPU_TEXTURETYPE_2D,
                         SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                         SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE |
                             SDL_GPU_TEXTUREUSAGE_SAMPLER);
    if (!rw_texture_.IsValid()) {
      return;
    }

    is_valid_ = true;
  }

  void DispatchCompute() {
    if (!is_valid_) {
      return;
    }
    auto command_buffer =
        SDL_AcquireGPUCommandBuffer(rw_texture_.texture.get().device);
    if (!command_buffer) {
      FML_LOG(ERROR) << "Could not create command buffer: " << SDL_GetError();
      return;
    }
    FML_DEFER(SDL_SubmitGPUCommandBuffer(command_buffer));

    SDL_GPUStorageTextureReadWriteBinding binding = {
        .texture = rw_texture_.texture.get().value};
    auto compute_pass =
        SDL_BeginGPUComputePass(command_buffer, &binding, 1u, nullptr, 0u);
    if (!compute_pass) {
      FML_LOG(ERROR) << "Could not create compute pass: " << SDL_GetError();
      return;
    }
    FML_DEFER(SDL_EndGPUComputePass(compute_pass));

    SDL_BindGPUComputePipeline(compute_pass, compute_pipeline_.get().value);
    const auto image_size =
        glm::ivec2(rw_texture_.info.width, rw_texture_.info.height);
    auto group_count = MakeGroupCount(image_size, thread_count_);
    SDL_DispatchGPUCompute(compute_pass, group_count.x, group_count.y, 1);
  }

  bool Draw(SDL_GPURenderPass* pass) override {
    if (!is_valid_) {
      return false;
    }

    DispatchCompute();

    SDL_BindGPUGraphicsPipeline(pass, render_pipeline_.get().value);
    SDL_GPUBufferBinding vtx_binding = {
        .buffer = render_vtx_buffer_.get().value,
    };
    SDL_BindGPUVertexBuffers(pass, 0, &vtx_binding, 1u);
    SDL_GPUTextureSamplerBinding frag_sampler_bindings = {
        .sampler = render_sampler_.get().value,
        .texture = rw_texture_.texture.get().value,
    };
    SDL_BindGPUFragmentSamplers(pass, 0, &frag_sampler_bindings, 1u);
    SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);

    return true;
  }

 private:
  glm::ivec2 thread_count_ = {32, 32};
  UniqueGPUComputePipeline compute_pipeline_;
  UniqueGPUGraphicsPipeline render_pipeline_;
  GPUTexture rw_texture_;
  UniqueGPUBuffer render_vtx_buffer_;
  UniqueGPUSampler render_sampler_;
  bool is_valid_ = false;
  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Compute);
};

}  // namespace ts
