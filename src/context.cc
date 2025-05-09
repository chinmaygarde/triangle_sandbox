#include "context.h"

namespace ts {

Context::Context(UniqueSDLWindow window) : window_(std::move(window)) {
  constexpr bool kIsDebuggingEnabled = true;
  // We are only wiring up MSL at the moment. There is no ability to select
  // drivers.
  device_.reset(::SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL,
                                      kIsDebuggingEnabled, NULL));
  FML_CHECK(device_.is_valid())
      << "Could not create GPU device: " << SDL_GetError();
  auto claimed = SDL_ClaimWindowForGPUDevice(device_.get(), window_.get());
  FML_CHECK(claimed) << "Could not claim device: " << SDL_GetError();

  color_format_ =
      SDL_GetGPUSwapchainTextureFormat(device_.get(), window_.get());
  depth_format_ = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
  color_samples_ = SDL_GPU_SAMPLECOUNT_4;
}

Context::~Context() {
  SDL_ReleaseWindowFromGPUDevice(device_.get(), window_.get());
}

const UniqueSDLWindow& Context::GetWindow() const {
  return window_;
}

const UniqueGPUDevice& Context::GetDevice() const {
  return device_;
}

SDL_GPUTextureFormat Context::GetColorFormat() const {
  return color_format_;
}

SDL_GPUTextureFormat Context::GetDepthFormat() const {
  return depth_format_;
}

SDL_GPUSampleCount Context::GetColorSamples() const {
  return color_samples_;
}

}  // namespace ts
