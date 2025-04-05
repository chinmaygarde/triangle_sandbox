#include "model.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "buffer.h"
#include "macros.h"
#include "model.slang.h"
#include "pipeline.h"
#include "shader.h"

namespace ts {

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
  }
  return {};
}

Model::Model(const UniqueGPUDevice& device, const fml::Mapping& mapping) {
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
      const std::vector<uint32_t> indices =
          ReadIndexBuffer(index_buffer.data.data() + index_acessor.byteOffset +
                              index_buffer_view.byteOffset,  //
                          index_acessor.count,               //
                          index_acessor.componentType        //
          );

      index_buffer_ = PerformHostToDeviceTransfer(device,                    //
                                                  indices,                   //
                                                  SDL_GPU_BUFFERUSAGE_INDEX  //
      );
      index_count_ = indices.size();

      std::vector<Vertex> vertices;

      if (auto position = primitive.attributes.find("POSITION");
          position != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model_.accessors[position->second];
        const tinygltf::BufferView& buffer_view =
            model_.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model_.buffers[buffer_view.buffer];
        if (accessor.type == TINYGLTF_TYPE_VEC3 &&
            accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
            buffer_view.byteStride == 0) {
          vertices.resize(glm::max(vertices.size(), accessor.count));
          for (size_t i = 0; i < accessor.count; i++) {
            memcpy(&vertices[i].position,
                   buffer.data.data() + accessor.byteOffset +
                       buffer_view.byteOffset + (sizeof(glm::vec3) * i),
                   sizeof(glm::vec3));
          }
        }
      }

      if (auto normal = primitive.attributes.find("NORMAL");
          normal != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model_.accessors[normal->second];
        const tinygltf::BufferView& buffer_view =
            model_.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model_.buffers[buffer_view.buffer];
        if (accessor.type == TINYGLTF_TYPE_VEC3 &&
            accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
            buffer_view.byteStride == 0) {
          vertices.resize(glm::max(vertices.size(), accessor.count));
          for (size_t i = 0; i < accessor.count; i++) {
            memcpy(&vertices[i].normal,
                   buffer.data.data() + accessor.byteOffset +
                       buffer_view.byteOffset + (sizeof(glm::vec3) * i),
                   sizeof(glm::vec3));
          }
        }
      }

      if (auto texcoord = primitive.attributes.find("TEXCOORD_0");
          texcoord != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model_.accessors[texcoord->second];
        const tinygltf::BufferView& buffer_view =
            model_.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model_.buffers[buffer_view.buffer];
        if (accessor.type == TINYGLTF_TYPE_VEC2 &&
            accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
            buffer_view.byteStride == 0) {
          vertices.resize(glm::max(vertices.size(), accessor.count));
          for (size_t i = 0; i < accessor.count; i++) {
            memcpy(&vertices[i].textureCoords,
                   buffer.data.data() + accessor.byteOffset +
                       buffer_view.byteOffset + (sizeof(glm::vec2) * i),
                   sizeof(glm::vec2));
          }
        }

        vertex_buffer_ = PerformHostToDeviceTransfer(
            device, vertices, SDL_GPU_BUFFERUSAGE_VERTEX);
      }

      // Only handle one for now.
      break;
    }
    break;
  }

  if (!index_buffer_.is_valid() || !vertex_buffer_.is_valid()) {
    return;
  }

  is_valid_ = true;
}

Model::~Model() {}

bool Model::IsValid() const {
  return is_valid_;
}

bool Model::ReadModel(const fml::Mapping& mapping) {
  tinygltf::TinyGLTF context;
  std::string error;
  std::string warning;
  if (!context.LoadBinaryFromMemory(&model_, &error, &warning,
                                    mapping.GetMapping(), mapping.GetSize())) {
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

bool Model::BuildPipeline(const UniqueGPUDevice& device) {
  auto code = fml::NonOwnedMapping{xxd_model_data, xxd_model_length};

  auto vs = ShaderBuilder{}
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetStage(SDL_GPU_SHADERSTAGE_VERTEX)
                .SetEntrypoint("VertexMain")
                .SetResourceCounts(0, 0, 0, 1u)
                .Build(device);
  auto fs = ShaderBuilder{}
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetStage(SDL_GPU_SHADERSTAGE_FRAGMENT)
                .SetEntrypoint("FragmentMain")
                .SetResourceCounts(0, 0, 0, 0)
                .Build(device);

  pipeline_ =
      GraphicsPipelineBuilder{}
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
          .SetSampleCount(SDL_GPU_SAMPLECOUNT_4)
          .SetCullMode(SDL_GPU_CULLMODE_BACK)
          .SetDepthStencilFormat(SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT)
          .SetDepthStencilState(SDL_GPUDepthStencilState{
              .compare_op = SDL_GPU_COMPAREOP_LESS,
              .enable_depth_test = true,
              .enable_depth_write = true,
          })
          .Build(device);
  if (!pipeline_.is_valid()) {
    return false;
  }
  return true;
}

bool Model::Draw(SDL_GPUCommandBuffer* command_buffer,
                 SDL_GPURenderPass* pass) {
  if (!IsValid()) {
    return false;
  }

  SDL_PushGPUDebugGroup(command_buffer, "Model");
  FML_DEFER(SDL_PopGPUDebugGroup(command_buffer));

  if (index_count_ == 0) {
    return true;
  }

  SDL_BindGPUGraphicsPipeline(pass, pipeline_.get().value);

  {
    const auto binding = SDL_GPUBufferBinding{
        .buffer = vertex_buffer_.get().value,
        .offset = 0u,
    };
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
  }

  {
    const auto binding = SDL_GPUBufferBinding{
        .buffer = index_buffer_.get().value,
        .offset = 0u,
    };
    SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
  }

  {
    glm::mat4 proj =
        glm::perspectiveLH_ZO(glm::radians(60.0), 800.0 / 600.0, 0.1, 1000.0);
    glm::mat4 view = glm::lookAtLH(glm::vec3{0.0, 0.0, -10.0},  // eye
                                   glm::vec3{0},                // center
                                   glm::vec3{0.0, 1.0, 0.0}     // up
    );
    auto mvp = proj * view;
    SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(mvp));
  }

  SDL_DrawGPUIndexedPrimitives(pass, index_count_, 1, 0, 0, 0);

  return true;
}

}  // namespace ts
