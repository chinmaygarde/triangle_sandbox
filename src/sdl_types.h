#pragma once

#include <SDL3/SDL_gpu.h>
#include <fml/unique_object.h>

namespace ts {

template <class T>
struct GPUDevicePair {
  SDL_GPUDevice* device = nullptr;
  T* value = nullptr;

  constexpr auto operator<=>(const GPUDevicePair<T>&) const = default;
};

template <class T>
void FreeSDLType(T* value);

template <class T>
void FreeSDLTypeWithDevice(const GPUDevicePair<T>& value);

template <class T>
struct UniqueSDLTypeTraits {
  static T* InvalidValue() { return nullptr; }

  static bool IsValid(const T* value) { return value != InvalidValue(); }

  static void Free(T* value) { FreeSDLType<T>(value); }
};

template <class T>
struct UniqueSDLTypeTraitsWithDevice {
  static GPUDevicePair<T> InvalidValue() { return {}; }

  static bool IsValid(const GPUDevicePair<T>& value) {
    return value != InvalidValue();
  }

  static void Free(const GPUDevicePair<T>& value) {
    FreeSDLTypeWithDevice(value);
  }
};

template <>
inline void FreeSDLType(SDL_GPUDevice* device) {
  SDL_DestroyGPUDevice(device);
}

template <>
inline void FreeSDLType(SDL_Window* window) {
  SDL_DestroyWindow(window);
}

template <>
inline void FreeSDLTypeWithDevice(
    const GPUDevicePair<SDL_GPUGraphicsPipeline>& value) {
  SDL_ReleaseGPUGraphicsPipeline(value.device, value.value);
}

template <>
inline void FreeSDLTypeWithDevice(
    const GPUDevicePair<SDL_GPUComputePipeline>& value) {
  SDL_ReleaseGPUComputePipeline(value.device, value.value);
}

template <>
inline void FreeSDLTypeWithDevice(const GPUDevicePair<SDL_GPUShader>& value) {
  SDL_ReleaseGPUShader(value.device, value.value);
}

template <>
inline void FreeSDLTypeWithDevice(
    const GPUDevicePair<SDL_GPUTransferBuffer>& value) {
  SDL_ReleaseGPUTransferBuffer(value.device, value.value);
}

template <>
inline void FreeSDLTypeWithDevice(const GPUDevicePair<SDL_GPUBuffer>& value) {
  SDL_ReleaseGPUBuffer(value.device, value.value);
}

template <>
inline void FreeSDLTypeWithDevice(const GPUDevicePair<SDL_GPUTexture>& value) {
  SDL_ReleaseGPUTexture(value.device, value.value);
}

template <class T>
using UniqueGPUObject = fml::UniqueObject<T*, UniqueSDLTypeTraits<T>>;

template <class T>
using UniqueGPUObjectWithDevice =
    fml::UniqueObject<GPUDevicePair<T>, UniqueSDLTypeTraitsWithDevice<T>>;

using UniqueSDLWindow = UniqueGPUObject<SDL_Window>;
using UniqueGPUDevice = UniqueGPUObject<SDL_GPUDevice>;
using UniqueGPUShader = UniqueGPUObjectWithDevice<SDL_GPUShader>;
using UniqueGPUGraphicsPipeline =
    UniqueGPUObjectWithDevice<SDL_GPUGraphicsPipeline>;
using UniqueGPUComputePipeline =
    UniqueGPUObjectWithDevice<SDL_GPUComputePipeline>;
using UniqueGPUTransferBuffer =
    UniqueGPUObjectWithDevice<SDL_GPUTransferBuffer>;
using UniqueGPUBuffer = UniqueGPUObjectWithDevice<SDL_GPUBuffer>;
using UniqueGPUTexture = UniqueGPUObjectWithDevice<SDL_GPUTexture>;

}  // namespace ts
