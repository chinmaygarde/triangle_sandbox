#pragma once

#include "context.h"
#include "drawable.h"
#include "sdl_types.h"

namespace ts {

class Triangle final : public Drawable {
 public:
  Triangle(const Context& ctx);

  bool Draw(const DrawContext& context) override;

 private:
  UniqueGPUGraphicsPipeline pipeline_;
  UniqueGPUBuffer vtx_buffer_;
  bool is_valid_ = false;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Triangle);
};

}  // namespace ts
