#include "shader.h"

namespace ts {

ShaderBuilder& ShaderBuilder::SetStage(SDL_GPUShaderStage stage) {
  stage_ = stage;
  return *this;
}

ShaderBuilder& ShaderBuilder::SetCode(fml::Mapping* code,
                                      SDL_GPUShaderFormat format) {
  code_ = code;
  format_ = format;
  return *this;
}

ShaderBuilder& ShaderBuilder::SetEntrypoint(std::string entrypoint) {
  entrypoint_ = std::move(entrypoint);
  return *this;
}

UniqueGPUShader ShaderBuilder::Build(const UniqueGPUDevice& device) const {
  SDL_GPUShaderCreateInfo info = {};
  if (code_) {
    info.code = code_->GetMapping();
    info.code_size = code_->GetSize();
  }
  info.format = format_;
  info.stage = stage_;
  if (!entrypoint_.empty()) {
    info.entrypoint = entrypoint_.data();
  }
  GPUDevicePair<SDL_GPUShader> shader;
  shader.device = device.get();
  shader.value = SDL_CreateGPUShader(device.get(), &info);
  if (!shader.value) {
    FML_LOG(ERROR) << "Could not create shader: " << SDL_GetError();
    return {};
  }
  return UniqueGPUShader{shader};
}

}  // namespace ts
