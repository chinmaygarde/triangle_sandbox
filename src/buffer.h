#pragma once

#include "sdl_types.h"

namespace ts {

[[nodiscard]]
UniqueGPUBuffer PerformHostToDeviceTransfer(const UniqueGPUDevice& device,
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
