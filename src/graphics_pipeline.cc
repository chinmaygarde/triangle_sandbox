#include "graphics_pipeline.h"

namespace ts {

GraphicsPipelineBuilder::GraphicsPipelineBuilder() {}

GraphicsPipelineBuilder::~GraphicsPipelineBuilder() {}

UniqueGPUGraphicsPipeline GraphicsPipelineBuilder::Build(
    const UniqueGPUDevice& device) const {
  SDL_GPUGraphicsPipelineCreateInfo info = {};
  if (vertex_shader_) {
    info.vertex_shader = vertex_shader_->get().value;
  }
  if (fragment_shader_) {
    info.fragment_shader = fragment_shader_->get().value;
  }
  info.primitive_type = primitive_type_;
  info.vertex_input_state.num_vertex_buffers = vertex_buffers_.size();
  info.vertex_input_state.vertex_buffer_descriptions = vertex_buffers_.data();
  info.vertex_input_state.num_vertex_attributes = vertex_attribs_.size();
  info.vertex_input_state.vertex_attributes = vertex_attribs_.data();
  info.target_info.color_target_descriptions = color_targets_.data();
  info.target_info.num_color_targets = color_targets_.size();
  info.multisample_state.sample_count = sample_count_;
  info.rasterizer_state.cull_mode = cull_mode_;
  info.depth_stencil_state = depth_stencil_;
  if (depth_stencil_format_ != SDL_GPU_TEXTUREFORMAT_INVALID) {
    info.target_info.has_depth_stencil_target = true;
    info.target_info.depth_stencil_format = depth_stencil_format_;
  }

  auto pipeline = SDL_CreateGPUGraphicsPipeline(device.get(), &info);
  if (!pipeline) {
    FML_LOG(ERROR) << "Could not create graphics pipeline: " << SDL_GetError();
    return {};
  }

  GPUDevicePair<SDL_GPUGraphicsPipeline> res = {};
  res.device = device.get();
  res.value = pipeline;
  return UniqueGPUGraphicsPipeline{res};
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetPrimitiveType(
    SDL_GPUPrimitiveType type) {
  primitive_type_ = type;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexShader(
    UniqueGPUShader* vertex_shader) {
  vertex_shader_ = vertex_shader;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetFragmentShader(
    UniqueGPUShader* fragment_shader) {
  fragment_shader_ = fragment_shader;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexBuffers(
    std::vector<SDL_GPUVertexBufferDescription> vertex_buffers) {
  vertex_buffers_ = std::move(vertex_buffers);
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexAttribs(
    std::vector<SDL_GPUVertexAttribute> vertex_attribs) {
  vertex_attribs_ = std::move(vertex_attribs);
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetColorTargets(
    std::vector<SDL_GPUColorTargetDescription> color_targets) {
  color_targets_ = std::move(color_targets);
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetSampleCount(
    SDL_GPUSampleCount sample_count) {
  sample_count_ = sample_count;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthStencilFormat(
    SDL_GPUTextureFormat depth_stencil_format) {
  depth_stencil_format_ = depth_stencil_format;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetDepthStencilState(
    SDL_GPUDepthStencilState depth_stencil) {
  depth_stencil_ = depth_stencil;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetCullMode(
    SDL_GPUCullMode cull_mode) {
  cull_mode_ = cull_mode;
  return *this;
}

}  // namespace ts
