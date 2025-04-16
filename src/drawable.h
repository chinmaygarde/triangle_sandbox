#pragma once

#include <fml/macros.h>
#include <glm/glm.hpp>
#include "sdl_types.h"

namespace ts {

struct DrawContext {
  glm::ivec2 viewport = {};
  SDL_GPUCommandBuffer* command_buffer = nullptr;
  SDL_GPURenderPass* pass = nullptr;

  float GetAspectRatio() const {
    const auto vp = glm::max(glm::vec2{viewport}, glm::vec2{1.0});
    return vp.x / vp.y;
  }
};

class Drawable {
 public:
  Drawable() = default;

  virtual ~Drawable() = default;

  virtual bool Draw(const DrawContext& context) = 0;

 private:
  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Drawable);
};

}  // namespace ts
