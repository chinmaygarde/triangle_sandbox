#include "buffer.h"

#include <fml/closure.h>

namespace ts {

struct ScopedCopyPass {
  ScopedCopyPass(SDL_GPUDevice* device) {
    auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer) {
      FML_LOG(ERROR) << "Could not acquire command buffer: " << SDL_GetError();
      return;
    }

    auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) {
      FML_LOG(ERROR) << "Could not create copy pass: " << SDL_GetError();
      SDL_CancelGPUCommandBuffer(command_buffer);
      return;
    }

    copy_pass_ = copy_pass;
    command_buffer_ = command_buffer;
  }

  ~ScopedCopyPass() {
    if (copy_pass_) {
      SDL_EndGPUCopyPass(copy_pass_);
      SDL_SubmitGPUCommandBuffer(command_buffer_);
    }
  }

  bool IsValid() const { return !!copy_pass_; }

  SDL_GPUCopyPass* GetPass() const { return copy_pass_; }

 private:
  SDL_GPUCommandBuffer* command_buffer_ = nullptr;
  SDL_GPUCopyPass* copy_pass_ = nullptr;
  bool is_valid_ = false;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(ScopedCopyPass);
};

GPUTexture CreateGPUTexture(SDL_GPUDevice* device,
                            glm::ivec3 dims,
                            SDL_GPUTextureType type,
                            SDL_GPUTextureFormat format,
                            SDL_GPUTextureUsageFlags usage,
                            Uint32 mip_levels,
                            SDL_GPUSampleCount sample_count) {
  SDL_GPUTextureCreateInfo info = {};
  info.type = type;
  info.format = format;
  info.usage = usage;
  info.width = dims.x;
  info.height = dims.y;
  info.layer_count_or_depth = dims.z;
  info.num_levels = mip_levels;
  info.sample_count = sample_count;
  auto texture = SDL_CreateGPUTexture(device, &info);
  if (!texture) {
    FML_LOG(ERROR) << "Could not create texture: " << SDL_GetError();
    return {};
  }
  UniqueGPUTexture::element_type res = {};
  res.device = device;
  res.value = texture;
  return {info, UniqueGPUTexture{res}};
}

UniqueGPUBuffer CreateGPUBuffer(SDL_GPUDevice* device,
                                Uint32 size,
                                SDL_GPUBufferUsageFlags usage) {
  if (size == 0) {
    FML_LOG(ERROR) << "Could not create zero sized buffer.";
    return {};
  }
  SDL_GPUBufferCreateInfo info = {};
  info.size = size;
  info.usage = usage;

  UniqueGPUBuffer::element_type element;
  element.device = device;
  element.value = SDL_CreateGPUBuffer(element.device, &info);

  if (!element.value) {
    FML_LOG(ERROR) << "Could not create graphics buffer: " << SDL_GetError();
    return {};
  }
  return UniqueGPUBuffer{element};
}

static UniqueGPUBuffer PerformHostToDeviceTransfer(
    const UniqueGPUTransferBuffer& xfer_buffer,
    Uint32 size,
    SDL_GPUBufferUsageFlags usage) {
  auto buffer = CreateGPUBuffer(xfer_buffer.get().device, size, usage);
  if (!buffer.is_valid()) {
    return {};
  }

  ScopedCopyPass copy_pass(xfer_buffer.get().device);
  if (!copy_pass.IsValid()) {
    return {};
  }

  const auto src = SDL_GPUTransferBufferLocation{
      .transfer_buffer = xfer_buffer.get().value,
      .offset = 0u,
  };
  const auto dst = SDL_GPUBufferRegion{
      .buffer = buffer.get().value,
      .offset = 0u,
      .size = size,
  };
  SDL_UploadToGPUBuffer(copy_pass.GetPass(), &src, &dst, false);
  return buffer;
}

UniqueGPUBuffer PerformHostToDeviceTransfer(const UniqueGPUDevice& device,
                                            const uint8_t* data,
                                            size_t size,
                                            SDL_GPUBufferUsageFlags usage) {
  auto xfer_buffer = PopulateGPUTransferBuffer(device.get(), data, size);
  if (!xfer_buffer.is_valid()) {
    return {};
  }
  return PerformHostToDeviceTransfer(xfer_buffer, size, usage);
}

UniqueGPUTransferBuffer PopulateGPUTransferBuffer(SDL_GPUDevice* device,
                                                  const uint8_t* data,
                                                  size_t data_size) {
  if (data_size == 0) {
    FML_LOG(ERROR) << "Could not create zero sized transfer buffer.";
    return {};
  }
  SDL_GPUTransferBufferCreateInfo info = {};
  info.size = data_size;
  info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  UniqueGPUTransferBuffer::element_type value;
  value.device = device;
  value.value = SDL_CreateGPUTransferBuffer(device, &info);
  if (!value.value) {
    FML_LOG(ERROR) << "Could not create transfer buffer: " << SDL_GetError();
    return {};
  }
  UniqueGPUTransferBuffer buffer(value);
  auto memory = SDL_MapGPUTransferBuffer(device, buffer.get().value, false);
  if (!memory) {
    FML_LOG(ERROR) << "Could not map buffer: " << SDL_GetError();
    return {};
  }
  FML_DEFER(SDL_UnmapGPUTransferBuffer(device, buffer.get().value));
  ::memcpy(memory, data, data_size);
  return buffer;
}

GPUTexture PerformHostToDeviceTransferTexture2D(SDL_GPUDevice* device,
                                                SDL_GPUTextureFormat format,
                                                glm::ivec2 dims,
                                                const uint8_t* data,
                                                size_t data_size) {
  auto texture = CreateGPUTexture(device,                          //
                                  glm::ivec3{dims.x, dims.y, 1u},  //
                                  SDL_GPU_TEXTURETYPE_2D,          //
                                  format,                          //
                                  SDL_GPU_TEXTUREUSAGE_SAMPLER     //
  );
  if (!texture.IsValid()) {
    return {};
  }
  auto xfer_buffer = PopulateGPUTransferBuffer(device, data, data_size);
  if (!xfer_buffer.is_valid()) {
    return {};
  }

  ScopedCopyPass copy_pass(device);
  if (!copy_pass.IsValid()) {
    return {};
  }

  const auto src = SDL_GPUTextureTransferInfo{
      .transfer_buffer = xfer_buffer.get().value,
      .pixels_per_row = static_cast<Uint32>(dims.x),
      .rows_per_layer = static_cast<Uint32>(dims.y),
  };

  const auto dst = SDL_GPUTextureRegion{
      .texture = texture.texture.get().value,
      .w = static_cast<Uint32>(dims.x),
      .h = static_cast<Uint32>(dims.y),
      .d = 1,
  };

  SDL_UploadToGPUTexture(copy_pass.GetPass(), &src, &dst, false);

  return texture;
}

}  // namespace ts
