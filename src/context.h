#pragma once

#include <fml/unique_object.h>
#include "sdl_types.h"

namespace ts {

class Context {
 public:
  Context(UniqueSDLWindow window);

  ~Context();

  const UniqueSDLWindow& GetWindow() const;

  const UniqueGPUDevice& GetDevice() const;

  SDL_GPUTextureFormat GetColorFormat() const;

  SDL_GPUTextureFormat GetDepthFormat() const;

  SDL_GPUSampleCount GetColorSamples() const;

 private:
  UniqueSDLWindow window_;
  UniqueGPUDevice device_;
  SDL_GPUTextureFormat color_format_ = SDL_GPU_TEXTUREFORMAT_INVALID;
  SDL_GPUTextureFormat depth_format_ = SDL_GPU_TEXTUREFORMAT_INVALID;
  SDL_GPUSampleCount color_samples_ = SDL_GPU_SAMPLECOUNT_1;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Context);
};

}  // namespace ts
