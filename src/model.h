#pragma once

#include <fml/macros.h>
#include <fml/mapping.h>
#include <tiny_gltf.h>
#include "model.slang.h"
#include "pipeline.h"
#include "shader.h"

namespace ts {

class Model {
 public:
  struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 textureCoords;
  };

  template <class To, class From>
  static std::vector<To> ReadIndexBuffer(const uint8_t* buffer,
                                         size_t item_count) {
    std::vector<To> result;
    result.resize(item_count);
    const auto from_buffer = reinterpret_cast<const From*>(buffer);
    for (size_t i = 0; i < item_count; i++) {
      result[i] = static_cast<To>(from_buffer[i]);
    }
    return result;
  }

  static std::vector<uint32_t> ReadIndexBuffer(const uint8_t* buffer,
                                               size_t item_count,
                                               int item_component_type) {
    switch (item_component_type) {
      case TINYGLTF_COMPONENT_TYPE_BYTE:
        return ReadIndexBuffer<uint32_t, int8_t>(buffer, item_count);
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return ReadIndexBuffer<uint32_t, uint8_t>(buffer, item_count);
      case TINYGLTF_COMPONENT_TYPE_SHORT:
        return ReadIndexBuffer<uint32_t, int16_t>(buffer, item_count);
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return ReadIndexBuffer<uint32_t, uint16_t>(buffer, item_count);
      case TINYGLTF_COMPONENT_TYPE_INT:
        return ReadIndexBuffer<uint32_t, int32_t>(buffer, item_count);
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        return ReadIndexBuffer<uint32_t, uint32_t>(buffer, item_count);
      case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return ReadIndexBuffer<uint32_t, float>(buffer, item_count);
      case TINYGLTF_COMPONENT_TYPE_DOUBLE:
        return ReadIndexBuffer<uint32_t, double>(buffer, item_count);
        break;
    }
    return {};
  }

  Model(const UniqueGPUDevice& device, const fml::Mapping& mapping) {
    if (!ReadModel(mapping)) {
      return;
    }
    if (!BuildPipeline(device)) {
      return;
    }

    for (const auto& mesh : model_.meshes) {
      for (const auto& primitive : mesh.primitives) {
        // Figure out indices.
        const auto& index_acessor = model_.accessors[primitive.indices];
        const auto& index_buffer_view =
            model_.bufferViews[index_acessor.bufferView];
        const auto& index_buffer = model_.buffers[index_buffer_view.buffer];
        const auto indices = ReadIndexBuffer(
            index_buffer.data.data() + index_acessor.byteOffset +
                index_buffer_view.byteOffset,  //
            index_acessor.count,               //
            index_acessor.componentType        //
        );

        std::vector<Vertex> vertices;

        // Figure out vertices.
        if (auto position = primitive.attributes.find("POSITION");
            position != primitive.attributes.end()) {
          const auto& accessor = model_.accessors[position->second];
          const auto& buffer_view = model_.bufferViews[accessor.bufferView];
          const auto& buffer = model_.buffers[buffer_view.buffer];
          vertices.resize(glm::max(vertices.size(), accessor.count));
          for (size_t i = 0; i < accessor.count; i++) {
          }
        }
      }
    }

    is_valid_ = true;
  }

  bool IsValid() const { return is_valid_; }

 private:
  bool is_valid_ = false;
  tinygltf::Model model_;
  UniqueGPUGraphicsPipeline pipeline_;

  bool ReadModel(const fml::Mapping& mapping) {
    tinygltf::TinyGLTF context;
    std::string error;
    std::string warning;
    if (!context.LoadBinaryFromMemory(&model_, &error, &warning,
                                      mapping.GetMapping(),
                                      mapping.GetSize())) {
      FML_LOG(ERROR) << "Could not load model";
      if (!error.empty()) {
        FML_LOG(ERROR) << "Error: " << error;
      }
      if (!warning.empty()) {
        FML_LOG(ERROR) << "Warning: " << warning;
      }
      return false;
    }
    return true;
  }

  bool BuildPipeline(const UniqueGPUDevice& device) {
    auto code = fml::NonOwnedMapping{xxd_model_data, xxd_model_length};

    auto vs = ShaderBuilder{}
                  .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                  .SetStage(SDL_GPU_SHADERSTAGE_VERTEX)
                  .SetEntrypoint("VertexMain")
                  .SetResourceCounts(0, 0, 0, 0)  // FIXME
                  .Build(device);
    auto fs = ShaderBuilder{}
                  .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                  .SetStage(SDL_GPU_SHADERSTAGE_FRAGMENT)
                  .SetEntrypoint("FragmentMain")
                  .SetResourceCounts(0, 0, 0, 0)  // FIXME
                  .Build(device);

    pipeline_ = GraphicsPipelineBuilder{}
                    .SetColorTargets({
                        SDL_GPUColorTargetDescription{
                            .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
                        },
                    })
                    .SetVertexShader(&vs)
                    .SetFragmentShader(&fs)
                    .SetPrimitiveType(SDL_GPU_PRIMITIVETYPE_TRIANGLELIST)
                    .SetVertexAttribs({
                        // Position
                        SDL_GPUVertexAttribute{
                            .buffer_slot = 0,
                            .location = 0,
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                            .offset = offsetof(Vertex, position),
                        },
                        // Normal
                        SDL_GPUVertexAttribute{
                            .buffer_slot = 0,
                            .location = 1,
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                            .offset = offsetof(Vertex, position),
                        },
                        // Texture Coords
                        SDL_GPUVertexAttribute{
                            .buffer_slot = 0,
                            .location = 2,
                            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                            .offset = offsetof(Vertex, position),
                        },
                    })
                    .SetVertexBuffers({
                        SDL_GPUVertexBufferDescription{
                            .slot = 0u,
                            .pitch = sizeof(Vertex),
                        },
                    })
                    .Build(device);
    if (!pipeline_.is_valid()) {
      return false;
    }
    return true;
  }

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Model);
};

}  // namespace ts
