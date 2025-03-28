#pragma once

#include <fml/macros.h>
#include "sdl_types.h"

namespace ts {

enum class PrimitiveType {
  kTriangleList = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
  kTriangleStrip = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
  kLineList = SDL_GPU_PRIMITIVETYPE_LINELIST,
  kLineStrip = SDL_GPU_PRIMITIVETYPE_LINESTRIP,
  kPointList = SDL_GPU_PRIMITIVETYPE_POINTLIST,
};

class GraphicsPipelineBuilder {
 public:
  GraphicsPipelineBuilder() {}

  GraphicsPipelineBuilder& SetPrimitiveType(PrimitiveType type) {
    primitive_type_ = type;
    return *this;
  }

  UniqueGPUGraphicsPipeline Build() const {
    SDL_GPUGraphicsPipelineCreateInfo info = {};
    if (vertex_shader_) {
      info.vertex_shader = vertex_shader_->get().shader;
    }
    if (fragment_shader_) {
      info.fragment_shader = fragment_shader_->get().shader;
    }
    info.primitive_type = static_cast<SDL_GPUPrimitiveType>(primitive_type_);
    return {};
  }

 private:
  UniqueGPUShader* vertex_shader_ = nullptr;
  UniqueGPUShader* fragment_shader_ = nullptr;
  PrimitiveType primitive_type_ = PrimitiveType::kTriangleList;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(GraphicsPipelineBuilder);
};

}  // namespace ts
