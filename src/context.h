#pragma once

#include <fml/unique_object.h>
#include "sdl_types.h"

namespace ts {

class Context {
 public:
  Context(UniqueSDLWindow window);

  const UniqueSDLWindow& GetWindow() const;

  const UniqueGPUDevice& GetDevice() const;

  SDL_GPUTextureFormat GetColorFormat() const;

 private:
  UniqueSDLWindow window_;
  UniqueGPUDevice device_;
  SDL_GPUTextureFormat color_format_ = SDL_GPU_TEXTUREFORMAT_INVALID;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Context);
};

}  // namespace ts
