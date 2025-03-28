#pragma once

#include <fml/macros.h>
#include "sdl_types.h"

namespace ts {

class GraphicsPipelineBuilder {
 public:
  GraphicsPipelineBuilder();

  GraphicsPipelineBuilder& SetPrimitiveType(SDL_GPUPrimitiveType type);

  GraphicsPipelineBuilder& SetVertexShader(UniqueGPUShader* vertex_shader);

  GraphicsPipelineBuilder& SetFragmentShader(UniqueGPUShader* fragment_shader);

  GraphicsPipelineBuilder& SetVertexBuffers(
      std::vector<SDL_GPUVertexBufferDescription> vertex_buffers);

  GraphicsPipelineBuilder& SetVertexAttribs(
      std::vector<SDL_GPUVertexAttribute> vertex_attribs);

  GraphicsPipelineBuilder& SetColorTargets(
      std::vector<SDL_GPUColorTargetDescription> color_targets);

  UniqueGPUGraphicsPipeline Build(const UniqueGPUDevice& device) const;

 private:
  UniqueGPUShader* vertex_shader_ = nullptr;
  UniqueGPUShader* fragment_shader_ = nullptr;
  SDL_GPUPrimitiveType primitive_type_ = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  std::vector<SDL_GPUVertexBufferDescription> vertex_buffers_;
  std::vector<SDL_GPUVertexAttribute> vertex_attribs_;
  std::vector<SDL_GPUColorTargetDescription> color_targets_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(GraphicsPipelineBuilder);
};

}  // namespace ts
