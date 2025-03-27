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

using UniqueGPUDevice =
    fml::UniqueObject<SDL_GPUDevice*, UniqueSDLTypeTraits<SDL_GPUDevice>>;
using UniqueSDLWindow =
    fml::UniqueObject<SDL_Window*, UniqueSDLTypeTraits<SDL_Window>>;

}  // namespace ts
