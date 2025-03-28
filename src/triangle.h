#pragma once

#include <SDL3/SDL_gpu.h>
#include <fml/logging.h>
#include <fml/mapping.h>
#include "sdl_types.h"
#include "shader.h"
#include "triangle_library.h"

namespace ts {

class Triangle {
 public:
  Triangle(const UniqueGPUDevice& device) {
    auto code = fml::NonOwnedMapping(xxd_triangle_library_data,
                                     xxd_triangle_library_length);

    auto vs = ShaderBuilder{}
                  .SetStage(ShaderStage::kVertex)
                  .SetCode(&code, ShaderFormat::kMSL)
                  .SetEntrypoint("VertexMain")
                  .Build(device);
    auto fs = ShaderBuilder{}
                  .SetStage(ShaderStage::kFragment)
                  .SetCode(&code, ShaderFormat::kMSL)
                  .SetEntrypoint("FragmentMain")
                  .Build(device);
  }

 private:
  UniqueGPUShader shader_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Triangle);
};

}  // namespace ts
