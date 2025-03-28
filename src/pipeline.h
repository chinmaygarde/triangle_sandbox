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

  GraphicsPipelineBuilder& SetVertexShader(UniqueGPUShader* vertex_shader) {
    vertex_shader_ = vertex_shader;
    return *this;
  }

  GraphicsPipelineBuilder& SetFragmentShader(UniqueGPUShader* fragment_shader) {
    fragment_shader_ = fragment_shader;
    return *this;
  }

  UniqueGPUGraphicsPipeline Build(const UniqueGPUDevice& device) const {
    SDL_GPUGraphicsPipelineCreateInfo info = {};
    if (vertex_shader_) {
      info.vertex_shader = vertex_shader_->get().shader;
    }
    if (fragment_shader_) {
      info.fragment_shader = fragment_shader_->get().shader;
    }
    info.primitive_type = static_cast<SDL_GPUPrimitiveType>(primitive_type_);
    info.vertex_input_state.num_vertex_buffers = vertex_buffers_.size();
    info.vertex_input_state.vertex_buffer_descriptions = vertex_buffers_.data();
    info.vertex_input_state.num_vertex_attributes = vertex_attribs_.size();
    info.vertex_input_state.vertex_attributes = vertex_attribs_.data();
    info.target_info.color_target_descriptions = color_targets_.data();
    info.target_info.num_color_targets = color_targets_.size();
    auto pipeline = SDL_CreateGPUGraphicsPipeline(device.get(), &info);
    if (!pipeline) {
      FML_LOG(ERROR) << "Could not create graphics pipeline: "
                     << SDL_GetError();
      return {};
    }
    GPUDeviceGraphicsPipeline res = {};
    res.device = device.get();
    res.pipeline = pipeline;
    return UniqueGPUGraphicsPipeline{res};
  }

  GraphicsPipelineBuilder& SetVertexBuffers(
      std::vector<SDL_GPUVertexBufferDescription> vertex_buffers) {
    vertex_buffers_ = std::move(vertex_buffers);
    return *this;
  }

  GraphicsPipelineBuilder& SetVertexAttribs(
      std::vector<SDL_GPUVertexAttribute> vertex_attribs) {
    vertex_attribs_ = std::move(vertex_attribs);
    return *this;
  }

  GraphicsPipelineBuilder& SetColorTargets(
      std::vector<SDL_GPUColorTargetDescription> color_targets) {
    color_targets_ = std::move(color_targets);
    return *this;
  }

 private:
  UniqueGPUShader* vertex_shader_ = nullptr;
  UniqueGPUShader* fragment_shader_ = nullptr;
  PrimitiveType primitive_type_ = PrimitiveType::kTriangleList;
  std::vector<SDL_GPUVertexBufferDescription> vertex_buffers_;
  std::vector<SDL_GPUVertexAttribute> vertex_attribs_;
  std::vector<SDL_GPUColorTargetDescription> color_targets_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(GraphicsPipelineBuilder);
};

}  // namespace ts
