#pragma once

#include <fml/macros.h>
#include "sdl_types.h"

namespace ts {

class Drawable {
 public:
  Drawable() = default;

  virtual ~Drawable() = default;

  virtual bool Draw(SDL_GPUCommandBuffer* command_buffer,
                    SDL_GPURenderPass* pass) = 0;

 private:
  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Drawable);
};

}  // namespace ts
