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
  auto command_buffer =
      SDL_AcquireGPUCommandBuffer(context_->GetDevice().get());
  if (!command_buffer) {
    FML_LOG(ERROR) << "Could not get command buffer: " << SDL_GetError();
    return false;
  }
  FML_DEFER(SDL_SubmitGPUCommandBuffer(command_buffer));
  SDL_GPUTexture* texture = nullptr;
  Uint32 texture_width = 0u;
  Uint32 texture_height = 0u;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(
          command_buffer, context_->GetWindow().get(), &texture, &texture_width,
          &texture_height)) {
    FML_LOG(ERROR) << "Could not acquire swapchain image: " << SDL_GetError();
    return false;
  }
  if (texture == NULL) {
    // The wait completed with failure.
    FML_LOG(ERROR) << "Acquired swapchain texture was invalid.";
    return false;
  }

  SDL_GPUColorTargetInfo color_info = {};
  color_info.texture = texture;
  color_info.clear_color = {1.0f, 0.0f, 1.0f, 1.0f};
  color_info.load_op = SDL_GPU_LOADOP_CLEAR;
  color_info.store_op = SDL_GPU_STOREOP_STORE;

  auto render_pass =
      SDL_BeginGPURenderPass(command_buffer, &color_info, 1, NULL);

  if (!render_pass) {
    FML_LOG(ERROR) << "Could not begin render pass: " << SDL_GetError();
    return false;
  }
  FML_DEFER(SDL_EndGPURenderPass(render_pass));

  for (auto& drawable : drawables_) {
    if (!drawable->Draw(render_pass)) {
      return false;
    }
  }
  return true;
}

}  // namespace ts
