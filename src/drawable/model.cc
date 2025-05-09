#include "model.h"

#include <tiny_gltf.h>
#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "buffer.h"
#include "graphics_pipeline.h"
#include "imgui.h"
#include "macros.h"
#include "model.slang.h"
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

Model::Model(const Context& ctx, const fml::Mapping& mapping) {
  if (!BuildPipeline(ctx)) {
    return;
  }

  auto model = ParseModel(mapping);
  if (!model) {
    return;
  }

  default_sampler_ = CreateSampler(
      ctx.GetDevice().get(),
      SDL_GPUSamplerCreateInfo{
          .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
          .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
          .min_filter = SDL_GPU_FILTER_LINEAR,
          .mag_filter = SDL_GPU_FILTER_LINEAR,
          .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
      });
  if (!default_sampler_.is_valid()) {
    FML_LOG(ERROR) << "Could not create default sampler.";
    return;
  }

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
        PerformHostToDeviceTransferTexture2D(ctx.GetDevice().get(),        //
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
        ctx.GetDevice().get(),
        SDL_GPUSamplerCreateInfo{
            .address_mode_u = AddressModeTinyGLTFToSDLGPU(sampler.wrapS),
            .address_mode_v = AddressModeTinyGLTFToSDLGPU(sampler.wrapT),
            .min_filter = FilterModeTinyGLTFToSDLGPU(sampler.minFilter),
            .mag_filter = FilterModeTinyGLTFToSDLGPU(sampler.magFilter),
            .mipmap_mode = MipmapModeGLTFToSDLGPU(sampler.minFilter),
        });
  }

  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;

  for (const auto& mesh : model->meshes) {
    for (const auto& primitive : mesh.primitives) {
      auto current_draw = DrawCall{
          .first_vertex = static_cast<Uint32>(vertices.size()),
          .first_index = static_cast<Uint32>(indices.size()),
      };

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

      {
        if (primitive.indices >= 0) {
          const auto& index_acessor = model->accessors.at(primitive.indices);
          const auto& index_buffer_view =
              model->bufferViews.at(index_acessor.bufferView);
          const auto& index_buffer =
              model->buffers.at(index_buffer_view.buffer);
          ReadIndexBuffer(indices,
                          index_buffer.data.data() + index_acessor.byteOffset +
                              index_buffer_view.byteOffset,  //
                          index_acessor.count,               //
                          index_acessor.componentType        //
          );
        } else {
          indices.reserve(indices.size() + current_vertices.size());
          for (size_t i = 0; i < current_vertices.size(); i++) {
            indices.push_back(i);
          }
        }
        current_draw.last_index = indices.size();
      }

      {
        if (primitive.material >= 0) {
          const auto& material = model->materials[primitive.material];
          if (auto index = material.pbrMetallicRoughness.baseColorTexture.index;
              index >= 0) {
            const auto& texture = model->textures[index];
            size_t texture_index =
                glm::clamp<int>(texture.source, 0u, textures_.size());
            size_t sampler_index =
                glm::clamp<int>(texture.sampler, 0u, samplers_.size());
            if (textures_.contains(texture_index)) {
              current_draw.base_color_texture = TextureBinding{
                  .texture = texture_index,

              };
              // The sampler is optional. A default will be picked if none is
              // specified.
              if (samplers_.contains(sampler_index)) {
                current_draw.base_color_texture->sampler = sampler_index;
              }
            }
          }
        }
      }

      std::ranges::move(current_vertices, std::back_inserter(vertices));
      draws_.push_back(current_draw);
    }
  }

  index_buffer_ = PerformHostToDeviceTransfer(ctx.GetDevice(),           //
                                              indices,                   //
                                              SDL_GPU_BUFFERUSAGE_INDEX  //
  );
  index_count_ = indices.size();
  vertex_buffer_ = PerformHostToDeviceTransfer(ctx.GetDevice(),            //
                                               vertices,                   //
                                               SDL_GPU_BUFFERUSAGE_VERTEX  //
  );

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
        PerformHostToDeviceTransferTexture2D(ctx.GetDevice().get(),        //
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
        ctx.GetDevice().get(),
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

bool Model::BuildPipeline(const Context& ctx) {
  auto code = fml::NonOwnedMapping{xxd_model_data, xxd_model_length};

  auto vs = ShaderBuilder{}
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetStage(SDL_GPU_SHADERSTAGE_VERTEX)
                .SetEntrypoint("VertexMain")
                .SetResourceCounts(0, 0, 0, 1u)
                .Build(ctx.GetDevice());
  auto fs = ShaderBuilder{}
                .SetCode(&code, SDL_GPU_SHADERFORMAT_MSL)
                .SetStage(SDL_GPU_SHADERSTAGE_FRAGMENT)
                .SetEntrypoint("FragmentMain")
                .SetResourceCounts(1u, 0, 0, 0)
                .Build(ctx.GetDevice());

  pipeline_ = GraphicsPipelineBuilder{}
                  .SetColorTargets({
                      SDL_GPUColorTargetDescription{
                          .format = ctx.GetColorFormat(),
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
                  .SetSampleCount(ctx.GetColorSamples())
                  .SetCullMode(SDL_GPU_CULLMODE_BACK)
                  .SetDepthStencilFormat(ctx.GetDepthFormat())
                  .SetDepthStencilState(SDL_GPUDepthStencilState{
                      .compare_op = SDL_GPU_COMPAREOP_GREATER,
                      .enable_depth_test = true,
                      .enable_depth_write = true,
                  })
                  .Build(ctx.GetDevice());
  if (!pipeline_.is_valid()) {
    return false;
  }
  return true;
}

bool Model::Draw(const DrawContext& context) {
  if (!IsValid()) {
    return false;
  }

  SDL_PushGPUDebugGroup(context.command_buffer, "Model");
  FML_DEFER(SDL_PopGPUDebugGroup(context.command_buffer));

  if (index_count_ == 0) {
    return true;
  }

  SDL_BindGPUGraphicsPipeline(context.pass, pipeline_.get().value);

  {
    const auto binding = SDL_GPUBufferBinding{
        .buffer = vertex_buffer_.get().value,
        .offset = 0u,
    };
    SDL_BindGPUVertexBuffers(context.pass, 0, &binding, 1);
  }

  {
    const auto binding = SDL_GPUBufferBinding{
        .buffer = index_buffer_.get().value,
        .offset = 0u,
    };
    SDL_BindGPUIndexBuffer(context.pass, &binding,
                           SDL_GPU_INDEXELEMENTSIZE_32BIT);
  }

  static float fov = 60;
  static glm::vec3 eye = glm::vec3{0.0, 0, -5.0};
  {
    ImGui::Begin("Viewport");
    ImGui::SliderFloat("FOV", &fov, 10, 180);
    ImGui::SliderFloat3("Eye", reinterpret_cast<float*>(&eye), -10, 10);
    ImGui::End();
  }

  {
    glm::mat4 proj = glm::perspective(glm::radians(fov),         //
                                      context.GetAspectRatio(),  //
                                      0.1f,                      //
                                      1000.0f                    //
    );
    glm::mat4 view = glm::lookAt(eye,                      // eye
                                 glm::vec3{0},             // center
                                 glm::vec3{0.0, 1.0, 0.0}  // up
    );
    glm::mat4 model = glm::mat4{1.0};
    auto mvp = proj * view * model;
    SDL_PushGPUVertexUniformData(context.command_buffer, 0, &mvp, sizeof(mvp));
  }

  for (const auto& draw : draws_) {
    if (!draw.base_color_texture.has_value()) {
      FML_LOG(ERROR) << "No base color.";
      continue;
    }

    const auto binding = SDL_GPUTextureSamplerBinding{
        .texture =
            textures_.at(draw.base_color_texture->texture).texture.get().value,
        .sampler = PickSampler(draw.base_color_texture->sampler),
    };
    SDL_BindGPUFragmentSamplers(context.pass, 0u, &binding, 1u);

    SDL_DrawGPUIndexedPrimitives(context.pass,                        //
                                 draw.last_index - draw.first_index,  //
                                 1u,                                  //
                                 draw.first_index,                    //
                                 draw.first_vertex,                   //
                                 0u                                   //
    );
  }

  return true;
}

SDL_GPUSampler* Model::PickSampler(std::optional<size_t> index) const {
  if (!index.has_value() || !samplers_.contains(index.value())) {
    return default_sampler_.get().value;
  }
  return samplers_.at(index.value()).get().value;
}

}  // namespace ts
