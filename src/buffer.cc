#include "buffer.h"

#include <fml/closure.h>

namespace ts {

static UniqueGPUBuffer PerformHostToDeviceTransfer(
    const UniqueGPUTransferBuffer& xfer_buffer,
    Uint32 size,
    SDL_GPUBufferUsageFlags usage) {
  SDL_GPUBufferCreateInfo info = {};
  info.size = size;
  info.usage = usage;

  UniqueGPUBuffer::element_type element;
  element.device = xfer_buffer.get().device;
  element.value = SDL_CreateGPUBuffer(element.device, &info);

  if (!element.value) {
    FML_LOG(ERROR) << "Could not create graphics buffer: " << SDL_GetError();
    return {};
  }

  UniqueGPUBuffer buffer(element);

  auto command_buffer = SDL_AcquireGPUCommandBuffer(buffer.get().device);
  if (!command_buffer) {
    FML_LOG(ERROR) << "Could not acquire command buffer: " << SDL_GetError();
    return {};
  }
  fml::ScopedCleanupClosure submit_command_buffer(
      [command_buffer]() { SDL_SubmitGPUCommandBuffer(command_buffer); });

  auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
  if (!copy_pass) {
    FML_LOG(ERROR) << "Could not create copy pass: " << SDL_GetError();
    return {};
  }
  fml::ScopedCleanupClosure end_pass(
      [copy_pass]() { SDL_EndGPUCopyPass(copy_pass); });

  {
    const auto src = SDL_GPUTransferBufferLocation{
        .transfer_buffer = xfer_buffer.get().value,
        .offset = 0u,
    };
    const auto dst = SDL_GPUBufferRegion{
        .buffer = buffer.get().value,
        .offset = 0u,
        .size = size,
    };
    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
  }
  return buffer;
}

UniqueGPUBuffer PerformHostToDeviceTransfer(const UniqueGPUDevice& device,
                                            const uint8_t* data,
                                            size_t size,
                                            SDL_GPUBufferUsageFlags usage) {
  SDL_GPUTransferBufferCreateInfo info = {};
  info.size = size;
  info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  UniqueGPUTransferBuffer::element_type value;
  value.device = device.get();
  value.value = SDL_CreateGPUTransferBuffer(device.get(), &info);
  if (!value.value) {
    FML_LOG(ERROR) << "Could not create transfer buffer: " << SDL_GetError();
    return {};
  }
  UniqueGPUTransferBuffer buffer(value);
  auto memory =
      SDL_MapGPUTransferBuffer(buffer.get().device, buffer.get().value, false);
  if (!memory) {
    FML_LOG(ERROR) << "Could not map buffer: " << SDL_GetError();
    return {};
  }
  ::memcpy(memory, data, size);
  SDL_UnmapGPUTransferBuffer(buffer.get().device, buffer.get().value);
  return PerformHostToDeviceTransfer(buffer, size, usage);
}

}  // namespace ts
