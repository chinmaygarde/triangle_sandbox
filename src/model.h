#pragma once

#include <fml/macros.h>
#include <fml/mapping.h>
#include <unordered_map>
#include "buffer.h"
#include "sdl_types.h"

namespace ts {

class Model {
 public:
  Model(const UniqueGPUDevice& device, const fml::Mapping& mapping);

  ~Model();

  bool IsValid() const;

  bool Draw(SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass* pass);

 private:
  struct DrawCall {
    Uint32 first_index = {};
    Uint32 last_index = {};
    Uint32 first_vertex = {};
  };
  UniqueGPUGraphicsPipeline pipeline_;
  UniqueGPUBuffer vertex_buffer_;
  UniqueGPUBuffer index_buffer_;
  Uint32 index_count_;
  std::unordered_map<size_t, GPUTexture> textures_;
  std::unordered_map<size_t, UniqueGPUSampler> samplers_;
  std::vector<DrawCall> draws_;
  bool is_valid_ = false;

  bool BuildPipeline(const UniqueGPUDevice& device);

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Model);
};

}  // namespace ts
