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
  info.num_samplers = num_samplers_;
  info.num_storage_textures = num_storage_textures_;
  info.num_storage_buffers = num_storage_buffers_;
  info.num_uniform_buffers = num_uniform_buffers_;
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

ShaderBuilder& ShaderBuilder::SetResourceCounts(Uint32 num_samplers,
                                                Uint32 num_storage_textures,
                                                Uint32 num_storage_buffers,
                                                Uint32 num_uniform_buffers) {
  num_samplers_ = num_samplers;
  num_storage_textures_ = num_storage_textures;
  num_storage_buffers_ = num_storage_buffers;
  num_uniform_buffers_ = num_uniform_buffers;
  return *this;
}

}  // namespace ts
