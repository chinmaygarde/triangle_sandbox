#pragma once

#include <fml/mapping.h>
#include <glm/glm.hpp>
#include "sdl_types.h"

namespace ts {

template <class T>
T MakeGroupCount(T data_size, T thread_count) {
  return (data_size + thread_count - T{1}) / thread_count;
}

struct ComputePipeline {
  UniqueGPUComputePipeline pipeline;
  glm::ivec3 thread_count = {};

  ComputePipeline() = default;

  ComputePipeline(UniqueGPUComputePipeline pipeline, glm::ivec3 dimensions)
      : pipeline(std::move(pipeline)), thread_count(dimensions) {}

  bool IsValid() const { return pipeline.is_valid(); }
};

class ComputePipelineBuilder {
 public:
  ComputePipelineBuilder();

  ~ComputePipelineBuilder();

  ComputePipeline Build(const UniqueGPUDevice& device) const;

  ComputePipelineBuilder& SetShader(fml::Mapping* shader,
                                    SDL_GPUShaderFormat format);

  ComputePipelineBuilder& SetDimensions(glm::ivec3 dimensions);

  ComputePipelineBuilder& SetResourceCounts(
      Uint32 num_samplers = 0,
      Uint32 num_readonly_storage_textures = 0,
      Uint32 num_readonly_storage_buffers = 0,
      Uint32 num_readwrite_storage_textures = 0,
      Uint32 num_readwrite_storage_buffers = 0,
      Uint32 num_uniform_buffers = 0);

  ComputePipelineBuilder& SetEntrypoint(std::string entrypoint);

 private:
  fml::Mapping* shader_ = nullptr;
  std::string entrypoint_;
  SDL_GPUShaderFormat format_ = SDL_GPU_SHADERFORMAT_MSL;
  glm::ivec3 dimensions_ = {1, 1, 1};
  Uint32 num_samplers_ = {};
  Uint32 num_readonly_storage_textures_ = {};
  Uint32 num_readonly_storage_buffers_ = {};
  Uint32 num_readwrite_storage_textures_ = {};
  Uint32 num_readwrite_storage_buffers_ = {};
  Uint32 num_uniform_buffers_ = {};

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(ComputePipelineBuilder);
};

}  // namespace ts
