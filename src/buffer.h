#pragma once

#include <glm/glm.hpp>
#include "sdl_types.h"

namespace ts {

[[nodiscard]]
UniqueGPUTexture CreateGPUTexture(
    SDL_GPUDevice* device,
    glm::ivec3 dims,
    SDL_GPUTextureType type,
    SDL_GPUTextureFormat format,
    SDL_GPUTextureUsageFlags usage,
    Uint32 mip_levels = 1u,
    SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1);

[[nodiscard]]
UniqueGPUBuffer CreateGPUBuffer(SDL_GPUDevice* device,
                                Uint32 size,
                                SDL_GPUBufferUsageFlags usage);

[[nodiscard]] UniqueGPUBuffer PerformHostToDeviceTransfer(
    const UniqueGPUDevice& device,
    const uint8_t* data,
    size_t size,
    SDL_GPUBufferUsageFlags usage);

template <class T>
[[nodiscard]] UniqueGPUBuffer PerformHostToDeviceTransfer(
    const UniqueGPUDevice& device,
    const std::vector<T>& buffer,
    SDL_GPUBufferUsageFlags usage) {
  return PerformHostToDeviceTransfer(
      device,                                           //
      reinterpret_cast<const uint8_t*>(buffer.data()),  //
      buffer.size() * sizeof(T),                        //
      usage                                             //
  );
}

}  // namespace ts
