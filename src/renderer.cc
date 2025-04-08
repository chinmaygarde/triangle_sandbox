#include "renderer.h"

#include <fml/closure.h>
#include "drawable/compute.h"
#include "drawable/model_renderer.h"
#include "drawable/triangle.h"

namespace ts {

Renderer::Renderer(std::shared_ptr<Context> context)
    : context_(std::move(context)) {
  drawables_.emplace_back(std::make_unique<Compute>(context_->GetDevice()));
  drawables_.emplace_back(std::make_unique<Triangle>(context_->GetDevice()));
  drawables_.emplace_back(
      std::make_unique<ModelRenderer>(context_->GetDevice(), "DamagedHelmet"));
}

bool Renderer::Render() {
  const auto& device = context_->GetDevice();
  auto command_buffer = SDL_AcquireGPUCommandBuffer(device.get());
  if (!command_buffer) {
    FML_LOG(ERROR) << "Could not get command buffer: " << SDL_GetError();
    return false;
  }
  FML_DEFER(SDL_SubmitGPUCommandBuffer(command_buffer));
  SDL_GPUTexture* resolve_texture = nullptr;
  Uint32 texture_width = 0u;
  Uint32 texture_height = 0u;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer,               //
                                             context_->GetWindow().get(),  //
                                             &resolve_texture,             //
                                             &texture_width,               //
                                             &texture_height               //
                                             )) {
    FML_LOG(ERROR) << "Could not acquire swapchain image: " << SDL_GetError();
    return false;
  }
  if (resolve_texture == NULL) {
    // The wait completed with failure.
    FML_LOG(ERROR) << "Acquired swapchain texture was invalid.";
    return false;
  }

  const auto texture_format = SDL_GetGPUSwapchainTextureFormat(
      device.get(), context_->GetWindow().get());

  auto color_texture = CreateGPUTexture(device.get(),                         //
                                        {texture_width, texture_height, 1u},  //
                                        SDL_GPU_TEXTURETYPE_2D,               //
                                        texture_format,                       //
                                        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,    //
                                        1u,                                   //
                                        SDL_GPU_SAMPLECOUNT_4                 //
  );

  auto depth_texture =
      CreateGPUTexture(device.get(),                               //
                       {texture_width, texture_height, 1u},        //
                       SDL_GPU_TEXTURETYPE_2D,                     //
                       SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT,    //
                       SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,  //
                       1u,                                         //
                       SDL_GPU_SAMPLECOUNT_4                       //
      );

  if (!color_texture.IsValid() || !depth_texture.IsValid()) {
    return false;
  }

  const auto color_info = SDL_GPUColorTargetInfo{
      .texture = color_texture.texture.get().value,
      .resolve_texture = resolve_texture,
      .clear_color = {1.0f, 0.0f, 1.0f, 1.0f},
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_RESOLVE,
  };

  const auto depth_stencil_info = SDL_GPUDepthStencilTargetInfo{
      .texture = depth_texture.texture.get().value,
      .clear_depth = 1.0f,
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_DONT_CARE,
      .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
      .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
      .clear_stencil = 0,
  };

  auto render_pass = SDL_BeginGPURenderPass(command_buffer,      //
                                            &color_info,         //
                                            1,                   //
                                            &depth_stencil_info  //
  );

  if (!render_pass) {
    FML_LOG(ERROR) << "Could not begin render pass: " << SDL_GetError();
    return false;
  }
  FML_DEFER(SDL_EndGPURenderPass(render_pass));

  DrawContext context = {
      .command_buffer = command_buffer,
      .pass = render_pass,
      .viewport = {texture_width, texture_height},
  };

  for (auto& drawable : drawables_) {
    if (!drawable->Draw(context)) {
      return false;
    }
  }
  return true;
}

}  // namespace ts
