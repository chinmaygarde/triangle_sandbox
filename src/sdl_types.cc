#include "sdl_types.h"

#include <fml/logging.h>

namespace ts {

UniqueGPUSampler CreateSampler(SDL_GPUDevice* device,
                               SDL_GPUSamplerCreateInfo info) {
  auto sampler = SDL_CreateGPUSampler(device, &info);
  if (!sampler) {
    FML_LOG(ERROR) << "Could not create sampler: " << SDL_GetError();
    return {};
  }
  UniqueGPUSampler ::element_type res = {};
  res.device = device;
  res.value = sampler;
  return UniqueGPUSampler{res};
}

}  // namespace ts
