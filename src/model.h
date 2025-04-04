#pragma once

#include <fml/macros.h>
#include <fml/mapping.h>
#include <tiny_gltf.h>
#include "sdl_types.h"

namespace ts {

class Model {
 public:
  Model(const UniqueGPUDevice& device, const fml::Mapping& mapping);

  ~Model();

  bool IsValid() const;

  bool Draw(SDL_GPURenderPass* pass);

 private:
  tinygltf::Model model_;
  UniqueGPUGraphicsPipeline pipeline_;
  UniqueGPUBuffer vertex_buffer_;
  UniqueGPUBuffer index_buffer_;
  Uint32 index_count_;
  bool is_valid_ = false;

  bool ReadModel(const fml::Mapping& mapping);

  bool BuildPipeline(const UniqueGPUDevice& device);

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Model);
};

}  // namespace ts
