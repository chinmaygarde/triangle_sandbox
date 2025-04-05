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

  auto texture = CreateGPUTexture(device.get(),                         //
                                  {texture_width, texture_height, 1u},  //
                                  SDL_GPU_TEXTURETYPE_2D,               //
                                  texture_format,                       //
                                  SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,    //
                                  1u,                                   //
                                  SDL_GPU_SAMPLECOUNT_4                 //
  );
  if (!texture.IsValid()) {
    return false;
  }

  SDL_GPUColorTargetInfo color_info = {};
  color_info.texture = texture.texture.get().value;
  color_info.resolve_texture = resolve_texture;
  color_info.clear_color = {1.0f, 0.0f, 1.0f, 1.0f};
  color_info.load_op = SDL_GPU_LOADOP_CLEAR;
  color_info.store_op = SDL_GPU_STOREOP_RESOLVE;

  auto render_pass =
      SDL_BeginGPURenderPass(command_buffer, &color_info, 1, NULL);

  if (!render_pass) {
    FML_LOG(ERROR) << "Could not begin render pass: " << SDL_GetError();
    return false;
  }
  FML_DEFER(SDL_EndGPURenderPass(render_pass));

  for (auto& drawable : drawables_) {
    if (!drawable->Draw(command_buffer, render_pass)) {
      return false;
    }
  }
  return true;
}

}  // namespace ts
