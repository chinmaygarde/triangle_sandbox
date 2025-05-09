#include "compute.h"

#include "compute.slang.h"
#include "graphics_pipeline.h"
#include "sampling.slang.h"
#include "shader.h"

namespace ts {

struct ComputeVertex {
  glm::vec4 position;
  glm::vec2 uv;
};

static UniqueGPUGraphicsPipeline CreateRenderPipeline(const Context& ctx) {
  auto code = fml::NonOwnedMapping{xxd_sampling_data, xxd_sampling_length};
  auto vs = ShaderBuilder{}
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetEntrypoint("SamplingVertexMain")
                .SetStage(SDL_GPU_SHADERSTAGE_VERTEX)
                .Build(ctx.GetDevice());

  auto fs = ShaderBuilder{}
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetEntrypoint("SamplingFragmentMain")
                .SetStage(SDL_GPU_SHADERSTAGE_FRAGMENT)
                .SetResourceCounts(1, 0, 0, 0)
                .Build(ctx.GetDevice());

  return GraphicsPipelineBuilder{}
      .SetVertexShader(&vs)
      .SetFragmentShader(&fs)
      .SetVertexAttribs({
          // Position
          SDL_GPUVertexAttribute{
              .buffer_slot = 0,
              .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
              .location = 0,
              .offset = offsetof(ComputeVertex, position),
          },
          // UV
          SDL_GPUVertexAttribute{
              .buffer_slot = 0,
              .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
              .location = 1,
              .offset = offsetof(ComputeVertex, uv),
          },
      })
      .SetVertexBuffers({
          SDL_GPUVertexBufferDescription{
              .slot = 0,
              .pitch = sizeof(ComputeVertex),
          },
      })
      .SetColorTargets({
          SDL_GPUColorTargetDescription{
              .format = ctx.GetColorFormat(),
          },
      })
      .SetPrimitiveType(SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP)
      .SetSampleCount(ctx.GetColorSamples())
      .SetDepthStencilFormat(ctx.GetDepthFormat())
      .Build(ctx.GetDevice());
}
Compute::Compute(const Context& ctx) {
  auto code = fml::NonOwnedMapping{xxd_compute_data, xxd_compute_length};
  compute_pipeline_ = ComputePipelineBuilder{}
                          .SetDimensions({32, 32, 1})
                          .SetShader(&code, SDL_GPU_SHADERFORMAT_MSL)
                          .SetEntrypoint("ComputeAdder")
                          .SetResourceCounts(0, 0, 0, 1, 0, 0)
                          .Build(ctx.GetDevice());
  if (!compute_pipeline_.IsValid()) {
    return;
  }

  render_pipeline_ = CreateRenderPipeline(ctx);
  if (!render_pipeline_.is_valid()) {
    return;
  }

  render_vtx_buffer_ =
      PerformHostToDeviceTransfer(ctx.GetDevice(),
                                  std::vector<ComputeVertex>{
                                      // 0
                                      ComputeVertex{
                                          .position = {-1, -1, 0, 1},
                                          .uv = {0, 1},
                                      },
                                      // 1
                                      ComputeVertex{
                                          .position = {-1, 1, 0, 1},
                                          .uv = {0, 0},
                                      },
                                      // 2
                                      ComputeVertex{
                                          .position = {1, -1, 0, 1},
                                          .uv = {1, 1},
                                      },
                                      // 3
                                      ComputeVertex{
                                          .position = {1, 1, 0, 1},
                                          .uv = {1, 0},
                                      },
                                  },
                                  SDL_GPU_BUFFERUSAGE_VERTEX);
  if (!render_vtx_buffer_.is_valid()) {
    return;
  }

  render_sampler_ = CreateSampler(
      ctx.GetDevice().get(),
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

  rw_texture_ = CreateGPUTexture(ctx.GetDevice().get(), {1200, 800, 1},
                                 SDL_GPU_TEXTURETYPE_2D,
                                 SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                 SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE |
                                     SDL_GPU_TEXTUREUSAGE_SAMPLER);
  if (!rw_texture_.IsValid()) {
    return;
  }

  is_valid_ = true;
}
Compute::~Compute() {}
void Compute::DispatchCompute() {
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

  SDL_BindGPUComputePipeline(compute_pass,
                             compute_pipeline_.pipeline.get().value);
  const auto image_size =
      glm::ivec2(rw_texture_.info.width, rw_texture_.info.height);
  const auto group_count =
      MakeGroupCount(image_size, glm::ivec2{compute_pipeline_.thread_count.x,
                                            compute_pipeline_.thread_count.y});
  SDL_DispatchGPUCompute(compute_pass, group_count.x, group_count.y, 1);
}
bool Compute::Draw(const DrawContext& context) {
  if (!is_valid_) {
    return false;
  }

  DispatchCompute();

  SDL_PushGPUDebugGroup(context.command_buffer, "ComputeDraw");
  FML_DEFER(SDL_PopGPUDebugGroup(context.command_buffer));

  SDL_BindGPUGraphicsPipeline(context.pass, render_pipeline_.get().value);
  SDL_GPUBufferBinding vtx_binding = {
      .buffer = render_vtx_buffer_.get().value,
  };
  SDL_BindGPUVertexBuffers(context.pass, 0, &vtx_binding, 1u);
  SDL_GPUTextureSamplerBinding frag_sampler_bindings = {
      .sampler = render_sampler_.get().value,
      .texture = rw_texture_.texture.get().value,
  };
  SDL_BindGPUFragmentSamplers(context.pass, 0, &frag_sampler_bindings, 1u);
  SDL_DrawGPUPrimitives(context.pass, 4, 1, 0, 0);

  return true;
}
}  // namespace ts
