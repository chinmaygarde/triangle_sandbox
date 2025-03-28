#pragma once

#include <fml/macros.h>
#include <fml/mapping.h>
#include "sdl_types.h"

namespace ts {

enum class ShaderStage {
  kVertex = SDL_GPU_SHADERSTAGE_VERTEX,
  kFragment = SDL_GPU_SHADERSTAGE_FRAGMENT,
};

enum class ShaderFormat {
  kMSL = SDL_GPU_SHADERFORMAT_MSL,
};

class ShaderBuilder {
 public:
  ShaderBuilder() {}

  ShaderBuilder& SetStage(ShaderStage stage) {
    stage_ = stage;
    return *this;
  }

  ShaderBuilder& SetCode(fml::Mapping* code, ShaderFormat format) {
    code_ = code;
    format_ = format;
    return *this;
  }

  ShaderBuilder& SetEntrypoint(std::string entrypoint) {
    entrypoint_ = std::move(entrypoint);
    return *this;
  }

  UniqueGPUShader Build(const UniqueGPUDevice& device) const {
    SDL_GPUShaderCreateInfo info = {};
    if (code_) {
      info.code = code_->GetMapping();
      info.code_size = code_->GetSize();
    }
    info.format = static_cast<SDL_GPUShaderFormat>(format_);
    info.stage = static_cast<SDL_GPUShaderStage>(stage_);
    if (!entrypoint_.empty()) {
      info.entrypoint = entrypoint_.data();
    }
    GPUDeviceShader shader;
    shader.device = device.get();
    shader.shader = SDL_CreateGPUShader(device.get(), &info);
    if (!shader.shader) {
      FML_LOG(ERROR) << "Could not create shader: " << SDL_GetError();
      return {};
    }
    return UniqueGPUShader{shader};
  }

 private:
  ShaderFormat format_ = ShaderFormat::kMSL;
  ShaderStage stage_ = ShaderStage::kVertex;
  fml::Mapping* code_ = nullptr;
  std::string entrypoint_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(ShaderBuilder);
};

}  // namespace ts
