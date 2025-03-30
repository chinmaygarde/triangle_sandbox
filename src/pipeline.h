#pragma once

#include <fml/macros.h>
#include <fml/mapping.h>
#include <glm/glm.hpp>
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

class ComputePipelineBuilder {
 public:
  ComputePipelineBuilder() {}

  UniqueGPUComputePipeline Build(const UniqueGPUDevice& device) const {
    SDL_GPUComputePipelineCreateInfo info = {};
    if (shader_) {
      info.code = shader_->GetMapping();
      info.code_size = shader_->GetSize();
    }
    info.entrypoint = entrypoint_.data();
    info.format = format_;
    info.threadcount_x = dimensions_.x;
    info.threadcount_y = dimensions_.y;
    info.threadcount_z = dimensions_.z;
    info.num_samplers = num_samplers_;
    info.num_readonly_storage_textures = num_readonly_storage_textures_;
    info.num_readonly_storage_buffers = num_readonly_storage_buffers_;
    info.num_readwrite_storage_textures = num_readwrite_storage_textures_;
    info.num_readwrite_storage_buffers = num_readwrite_storage_buffers_;
    info.num_uniform_buffers = num_uniform_buffers_;
    auto pipeline = SDL_CreateGPUComputePipeline(device.get(), &info);
    if (!pipeline) {
      FML_LOG(ERROR) << "Could not create compute pipeline: " << SDL_GetError();
      return {};
    }
    UniqueGPUComputePipeline::element_type res = {};
    res.device = device.get();
    res.value = pipeline;
    return UniqueGPUComputePipeline{res};
  }

  ComputePipelineBuilder& SetShader(fml::Mapping* shader,
                                    SDL_GPUShaderFormat format) {
    shader_ = shader;
    format_ = format;
    return *this;
  }

  ComputePipelineBuilder& SetDimensions(glm::ivec3 dimensions) {
    dimensions_ = dimensions;
    return *this;
  }

  ComputePipelineBuilder& SetResourceCounts(
      Uint32 num_samplers = 0,
      Uint32 num_readonly_storage_textures = 0,
      Uint32 num_readonly_storage_buffers = 0,
      Uint32 num_readwrite_storage_textures = 0,
      Uint32 num_readwrite_storage_buffers = 0,
      Uint32 num_uniform_buffers = 0) {
    num_samplers_ = num_samplers;
    num_readonly_storage_textures_ = num_readonly_storage_textures;
    num_readonly_storage_buffers_ = num_readonly_storage_buffers;
    num_readwrite_storage_textures_ = num_readwrite_storage_textures;
    num_readwrite_storage_buffers_ = num_readwrite_storage_buffers;
    num_uniform_buffers_ = num_uniform_buffers;
    return *this;
  }

  ComputePipelineBuilder& SetEntrypoint(std::string entrypoint) {
    entrypoint_ = std::move(entrypoint);
    return *this;
  }

 private:
  fml::Mapping* shader_ = nullptr;
  std::string entrypoint_;
  SDL_GPUShaderFormat format_ = SDL_GPU_SHADERFORMAT_MSL;
  glm::ivec3 dimensions_ = {};
  Uint32 num_samplers_ = {};
  Uint32 num_readonly_storage_textures_ = {};
  Uint32 num_readonly_storage_buffers_ = {};
  Uint32 num_readwrite_storage_textures_ = {};
  Uint32 num_readwrite_storage_buffers_ = {};
  Uint32 num_uniform_buffers_ = {};

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(ComputePipelineBuilder);
};

}  // namespace ts
