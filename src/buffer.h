#pragma once

#include <glm/glm.hpp>
#include "macros.h"
#include "sdl_types.h"

namespace ts {

struct GPUTexture {
  SDL_GPUTextureCreateInfo info = {};
  UniqueGPUTexture texture;

  GPUTexture() = default;

  GPUTexture(SDL_GPUTextureCreateInfo info, UniqueGPUTexture texture)
      : info(info), texture(std::move(texture)) {}

  bool IsValid() const { return texture.is_valid(); }
};

[[nodiscard]]
GPUTexture CreateGPUTexture(
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

[[nodiscard]]
UniqueGPUTransferBuffer PopulateGPUTransferBuffer(SDL_GPUDevice* device,
                                                  const uint8_t* data,
                                                  size_t data_size);

[[nodiscard]] GPUTexture PerformHostToDeviceTransferTexture2D(
    SDL_GPUDevice* device,
    SDL_GPUTextureFormat format,
    glm::ivec2 dims,
    const uint8_t* data,
    size_t data_size);

}  // namespace ts
