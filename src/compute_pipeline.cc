#include "compute_pipeline.h"

namespace ts {

ComputePipelineBuilder::ComputePipelineBuilder() {}

ComputePipelineBuilder::~ComputePipelineBuilder() {}

ComputePipeline ComputePipelineBuilder::Build(
    const UniqueGPUDevice& device) const {
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
  return ComputePipeline(UniqueGPUComputePipeline{res}, dimensions_);
}

ComputePipelineBuilder& ComputePipelineBuilder::SetShader(
    fml::Mapping* shader,
    SDL_GPUShaderFormat format) {
  shader_ = shader;
  format_ = format;
  return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::SetDimensions(
    glm::ivec3 dimensions) {
  dimensions_ = dimensions;
  return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::SetResourceCounts(
    Uint32 num_samplers,
    Uint32 num_readonly_storage_textures,
    Uint32 num_readonly_storage_buffers,
    Uint32 num_readwrite_storage_textures,
    Uint32 num_readwrite_storage_buffers,
    Uint32 num_uniform_buffers) {
  num_samplers_ = num_samplers;
  num_readonly_storage_textures_ = num_readonly_storage_textures;
  num_readonly_storage_buffers_ = num_readonly_storage_buffers;
  num_readwrite_storage_textures_ = num_readwrite_storage_textures;
  num_readwrite_storage_buffers_ = num_readwrite_storage_buffers;
  num_uniform_buffers_ = num_uniform_buffers;
  return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::SetEntrypoint(
    std::string entrypoint) {
  entrypoint_ = std::move(entrypoint);
  return *this;
}

}  // namespace ts
