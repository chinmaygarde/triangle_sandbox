#pragma once

#include <fml/closure.h>
#include <fml/mapping.h>
#include "buffer.h"
#include "compute_pipeline.h"
#include "context.h"
#include "drawable.h"
#include "sdl_types.h"

namespace ts {

class Compute final : public Drawable {
 public:
  Compute(const Context& ctx);

  ~Compute();

  bool Draw(const DrawContext& context) override;

 private:
  ComputePipeline compute_pipeline_;
  UniqueGPUGraphicsPipeline render_pipeline_;
  GPUTexture rw_texture_;
  UniqueGPUBuffer render_vtx_buffer_;
  UniqueGPUSampler render_sampler_;
  bool is_valid_ = false;

  void DispatchCompute();

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Compute);
};

}  // namespace ts
