#include "model.h"

#include <tiny_gltf.h>
#include <algorithm>
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

template <class From>
static void ReadIndexBuffer(std::vector<uint32_t>& indices,
                            const uint8_t* buffer,
                            size_t item_count) {
  const auto initial_count = indices.size();
  indices.resize(initial_count + item_count);
  const auto from_buffer = reinterpret_cast<const From*>(buffer);
  for (size_t i = 0; i < item_count; i++) {
    indices[i + initial_count] = static_cast<uint32_t>(from_buffer[i]);
  }
}

static void ReadIndexBuffer(std::vector<uint32_t>& indices,
                            const uint8_t* buffer,
                            size_t item_count,
                            int item_component_type) {
  switch (item_component_type) {
    case TINYGLTF_COMPONENT_TYPE_BYTE:
      ReadIndexBuffer<int8_t>(indices, buffer, item_count);
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
      ReadIndexBuffer<uint8_t>(indices, buffer, item_count);
      break;
    case TINYGLTF_COMPONENT_TYPE_SHORT:
      ReadIndexBuffer<int16_t>(indices, buffer, item_count);
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      ReadIndexBuffer<uint16_t>(indices, buffer, item_count);
      break;
    case TINYGLTF_COMPONENT_TYPE_INT:
      ReadIndexBuffer<int32_t>(indices, buffer, item_count);
      break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
      ReadIndexBuffer<uint32_t>(indices, buffer, item_count);
      break;
  }
}

static std::unique_ptr<tinygltf::Model> ParseModel(
    const fml::Mapping& mapping) {
  tinygltf::TinyGLTF context;
  std::string error;
  std::string warning;
  auto model = std::make_unique<tinygltf::Model>();
  if (!context.LoadBinaryFromMemory(model.get(), &error, &warning,
                                    mapping.GetMapping(), mapping.GetSize())) {
    FML_LOG(ERROR) << "Could not load model";
    if (!error.empty()) {
      FML_LOG(ERROR) << "Error: " << error;
    }
    if (!warning.empty()) {
      FML_LOG(ERROR) << "Warning: " << warning;
    }
    return nullptr;
  }
  return model;
}

static std::optional<SDL_GPUTextureFormat> PickFormat(int component_count,
                                                      int component_type,
                                                      int bits_per_pixel) {
  if (component_count == 4u &&
      component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
      bits_per_pixel == 8) {
    return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  }
  return std::nullopt;
}

static SDL_GPUSamplerAddressMode AddressModeTinyGLTFToSDLGPU(int val) {
  switch (val) {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
      return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
      return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
      return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
  }
  return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
}

static SDL_GPUFilter FilterModeTinyGLTFToSDLGPU(int val) {
  switch (val) {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
      return SDL_GPU_FILTER_NEAREST;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
      return SDL_GPU_FILTER_LINEAR;
  }
  return SDL_GPU_FILTER_NEAREST;
}

static SDL_GPUSamplerMipmapMode MipmapModeGLTFToSDLGPU(int val) {
  switch (val) {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
      return SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
      return SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
  }
  return SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
}

bool ReadVertexAttribute(std::vector<Vertex>& vertices,
                         const tinygltf::Model& model,
                         int attribute,
                         size_t field_offset,
                         size_t field_size,
                         int check_type,
                         int check_component_type) {
  const tinygltf::Accessor& accessor = model.accessors[attribute];
  const tinygltf::BufferView& buffer_view =
      model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
  if (accessor.type != check_type &&
      accessor.componentType != check_component_type) {
    return false;
  }
  const auto stride = accessor.ByteStride(buffer_view);
  vertices.resize(std::max(vertices.size(), accessor.count));
  const auto* data_ptr =
      buffer.data.data() + accessor.byteOffset + buffer_view.byteOffset;
  for (size_t i = 0; i < accessor.count; i++) {
    std::memcpy(reinterpret_cast<uint8_t*>(&vertices[i]) + field_offset,
                data_ptr + stride * i, field_size);
  }
  return true;
}

Model::Model(const UniqueGPUDevice& device, const fml::Mapping& mapping) {
  if (!BuildPipeline(device)) {
    return;
  }

  auto model = ParseModel(mapping);
  if (!model) {
    return;
  }

  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;

  for (const auto& mesh : model->meshes) {
    for (const auto& primitive : mesh.primitives) {
      auto current_draw = DrawCall{
          .first_vertex = static_cast<Uint32>(vertices.size()),
          .first_index = static_cast<Uint32>(indices.size()),
      };

      // Figure out indices.
      {
        const auto& index_acessor = model->accessors[primitive.indices];
        const auto& index_buffer_view =
            model->bufferViews[index_acessor.bufferView];
        const auto& index_buffer = model->buffers[index_buffer_view.buffer];
        ReadIndexBuffer(indices,
                        index_buffer.data.data() + index_acessor.byteOffset +
                            index_buffer_view.byteOffset,  //
                        index_acessor.count,               //
                        index_acessor.componentType        //
        );
        current_draw.last_index = indices.size();
      }

      std::vector<Vertex> current_vertices;

      if (auto position = primitive.attributes.find("POSITION");
          position != primitive.attributes.end()) {
        ReadVertexAttribute(current_vertices,              //
                            *model,                        //
                            position->second,              //
                            offsetof(Vertex, position),    //
                            sizeof(Vertex::position),      //
                            TINYGLTF_TYPE_VEC3,            //
                            TINYGLTF_COMPONENT_TYPE_FLOAT  //
        );
      }

      if (auto normal = primitive.attributes.find("NORMAL");
          normal != primitive.attributes.end()) {
        ReadVertexAttribute(current_vertices,              //
                            *model,                        //
                            normal->second,                //
                            offsetof(Vertex, normal),      //
                            sizeof(Vertex::normal),        //
                            TINYGLTF_TYPE_VEC3,            //
                            TINYGLTF_COMPONENT_TYPE_FLOAT  //
        );
      }

      if (auto texcoord = primitive.attributes.find("TEXCOORD_0");
          texcoord != primitive.attributes.end()) {
        ReadVertexAttribute(current_vertices,                 //
                            *model,                           //
                            texcoord->second,                 //
                            offsetof(Vertex, textureCoords),  //
                            sizeof(Vertex::textureCoords),    //
                            TINYGLTF_TYPE_VEC2,               //
                            TINYGLTF_COMPONENT_TYPE_FLOAT     //
        );
      }

      std::ranges::move(current_vertices, std::back_inserter(vertices));
      draws_.push_back(current_draw);
    }
  }

  index_buffer_ = PerformHostToDeviceTransfer(device,                    //
                                              indices,                   //
                                              SDL_GPU_BUFFERUSAGE_INDEX  //
  );
  index_count_ = indices.size();
  vertex_buffer_ =
      PerformHostToDeviceTransfer(device, vertices, SDL_GPU_BUFFERUSAGE_VERTEX);

  // Handle images.
  for (size_t i = 0, count = model->images.size(); i < count; i++) {
    const auto& image = model->images[i];
    auto format = PickFormat(image.component, image.pixel_type, image.bits);
    if (!format.has_value()) {
      FML_LOG(ERROR) << "Could not find format for image.";
      continue;
    }
    const auto data_size = SDL_CalculateGPUTextureFormatSize(
        format.value(), image.width, image.height, 1u);
    if (data_size > image.image.size()) {
      FML_LOG(ERROR) << "Prevented OOB access for image data.";
      continue;
    }
    auto texture =
        PerformHostToDeviceTransferTexture2D(device.get(),                 //
                                             format.value(),               //
                                             {image.width, image.height},  //
                                             image.image.data(),           //
                                             data_size                     //
        );
    textures_[i] = std::move(texture);
  }

  // Handle samplers.
  for (size_t i = 0, count = model->samplers.size(); i < count; i++) {
    const auto& sampler = model->samplers[i];
    samplers_[i] = CreateSampler(
        device.get(),
        SDL_GPUSamplerCreateInfo{
            .address_mode_u = AddressModeTinyGLTFToSDLGPU(sampler.wrapS),
            .address_mode_v = AddressModeTinyGLTFToSDLGPU(sampler.wrapT),
            .min_filter = FilterModeTinyGLTFToSDLGPU(sampler.minFilter),
            .mag_filter = FilterModeTinyGLTFToSDLGPU(sampler.magFilter),
            .mipmap_mode = MipmapModeGLTFToSDLGPU(sampler.minFilter),
        });
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
                .SetResourceCounts(1u, 0, 0, 0)
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
                  .offset = offsetof(Vertex, normal),
              },
              // Texture Coords
              SDL_GPUVertexAttribute{
                  .buffer_slot = 0,
                  .location = 2,
                  .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                  .offset = offsetof(Vertex, textureCoords),
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
    glm::mat4 view = glm::lookAtLH(glm::vec3{0.0, 0.0, -5.0},  // eye
                                   glm::vec3{0},               // center
                                   glm::vec3{0.0, 1.0, 0.0}    // up
    );
    auto mvp = proj * view;
    SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(mvp));
  }

  {
    const auto binding = SDL_GPUTextureSamplerBinding{
        .texture = textures_.at(0).texture.get().value,
        .sampler = samplers_.at(0).get().value,
    };
    SDL_BindGPUFragmentSamplers(pass, 0u, &binding, 1u);
  }

  for (const auto& draw : draws_) {
    SDL_DrawGPUIndexedPrimitives(pass,                                //
                                 draw.last_index - draw.first_index,  //
                                 1u,                                  //
                                 draw.first_index,                    //
                                 draw.first_vertex,                   //
                                 0u                                   //
    );
  }

  return true;
}

}  // namespace ts
