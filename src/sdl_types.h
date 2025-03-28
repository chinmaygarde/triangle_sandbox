#pragma once

#include <SDL3/SDL_gpu.h>
#include <fml/unique_object.h>

namespace ts {

template <class T>
void FreeSDLType(T* value);

template <class T>
struct UniqueSDLTypeTraits {
  static T* InvalidValue() { return nullptr; }

  static bool IsValid(const T* value) { return value != InvalidValue(); }

  static void Free(T* value) { FreeSDLType<T>(value); }
};

template <>
inline void FreeSDLType(SDL_GPUDevice* device) {
  SDL_DestroyGPUDevice(device);
}

template <>
inline void FreeSDLType(SDL_Window* window) {
  SDL_DestroyWindow(window);
}

struct GPUDeviceShader {
  SDL_GPUDevice* device = nullptr;
  SDL_GPUShader* shader = nullptr;

  constexpr auto operator<=>(const GPUDeviceShader&) const = default;
};

struct GPUDeviceShaderTraits {
  static GPUDeviceShader InvalidValue() { return {}; }

  static bool IsValid(const GPUDeviceShader& value) {
    return value != InvalidValue();
  }

  static void Free(const GPUDeviceShader& value) {
    SDL_ReleaseGPUShader(value.device, value.shader);
  }
};

struct GPUDeviceGraphicsPipeline {
  SDL_GPUDevice* device = nullptr;
  SDL_GPUGraphicsPipeline* pipeline = nullptr;

  constexpr auto operator<=>(const GPUDeviceGraphicsPipeline&) const = default;
};

struct GPUDeviceGraphicsPipelineTraits {
  static GPUDeviceGraphicsPipeline InvalidValue() { return {}; }

  static bool IsValid(const GPUDeviceGraphicsPipeline& value) {
    return value != InvalidValue();
  }

  static void Free(const GPUDeviceGraphicsPipeline& value) {
    SDL_ReleaseGPUGraphicsPipeline(value.device, value.pipeline);
  }
};

using UniqueGPUDevice =
    fml::UniqueObject<SDL_GPUDevice*, UniqueSDLTypeTraits<SDL_GPUDevice>>;
using UniqueSDLWindow =
    fml::UniqueObject<SDL_Window*, UniqueSDLTypeTraits<SDL_Window>>;
using UniqueGPUShader =
    fml::UniqueObject<GPUDeviceShader, GPUDeviceShaderTraits>;
using UniqueGPUGraphicsPipeline =
    fml::UniqueObject<GPUDeviceGraphicsPipeline,
                      GPUDeviceGraphicsPipelineTraits>;

}  // namespace ts
