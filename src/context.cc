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
}

const UniqueSDLWindow& Context::GetWindow() const {
  return window_;
}

const UniqueGPUDevice& Context::GetDevice() const {
  return device_;
}

}  // namespace ts
