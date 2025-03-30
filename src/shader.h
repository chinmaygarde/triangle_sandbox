#pragma once

#include <fml/macros.h>
#include <fml/mapping.h>
#include "sdl_types.h"

namespace ts {

class ShaderBuilder {
 public:
  ShaderBuilder() {}

  ShaderBuilder& SetStage(SDL_GPUShaderStage stage);

  ShaderBuilder& SetCode(fml::Mapping* code, SDL_GPUShaderFormat format);

  ShaderBuilder& SetEntrypoint(std::string entrypoint);

  ShaderBuilder& SetResourceCounts(Uint32 num_samplers,
                                   Uint32 num_storage_textures,
                                   Uint32 num_storage_buffers,
                                   Uint32 num_uniform_buffers);

  UniqueGPUShader Build(const UniqueGPUDevice& device) const;

 private:
  SDL_GPUShaderFormat format_ = SDL_GPU_SHADERFORMAT_MSL;
  SDL_GPUShaderStage stage_ = SDL_GPU_SHADERSTAGE_VERTEX;
  fml::Mapping* code_ = nullptr;
  std::string entrypoint_;
  Uint32 num_samplers_ = {};
  Uint32 num_storage_textures_ = {};
  Uint32 num_storage_buffers_ = {};
  Uint32 num_uniform_buffers_ = {};

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(ShaderBuilder);
};

}  // namespace ts
