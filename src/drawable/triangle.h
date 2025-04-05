#pragma once

#include "drawable.h"
#include "sdl_types.h"

namespace ts {

class Triangle final : public Drawable {
 public:
  Triangle(const UniqueGPUDevice& device);

  bool Draw(SDL_GPUCommandBuffer* command_buffer,
            SDL_GPURenderPass* pass) override;

 private:
  UniqueGPUGraphicsPipeline pipeline_;
  UniqueGPUBuffer vtx_buffer_;
  bool is_valid_ = false;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Triangle);
};

}  // namespace ts
