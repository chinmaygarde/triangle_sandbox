#include "renderer.h"

#include <fml/closure.h>
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "drawable/compute.h"
#include "drawable/model_renderer.h"
#include "drawable/triangle.h"
#include "imgui.h"

namespace ts {

Renderer::Renderer(std::shared_ptr<Context> context)
    : context_(std::move(context)) {
  drawables_.emplace_back(std::make_unique<Compute>(*context_));
  drawables_.emplace_back(std::make_unique<Triangle>(*context_));
  drawables_.emplace_back(
      std::make_unique<ModelRenderer>(*context_, "DamagedHelmet"));

  StartupIMGUI();
}

Renderer::~Renderer() {
  ShutdownIMGUI();
}

bool Renderer::Render() {
  BeginIMGUIFrame();
  if (auto texture = RenderOnce()) {
    EndIMGUIFrame(texture);
  }
  return true;
}

void Renderer::BeginIMGUIFrame() {
  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  ImGui::ShowDemoWindow(NULL);
}

void Renderer::EndIMGUIFrame(SDL_GPUTexture* texture) {
  ImGui::Render();

  auto command_buffer =
      SDL_AcquireGPUCommandBuffer(context_->GetDevice().get());
  FML_DEFER(SDL_SubmitGPUCommandBuffer(command_buffer));

  auto draw_data = ImGui::GetDrawData();
  ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

  SDL_GPUColorTargetInfo color = {
      .load_op = SDL_GPU_LOADOP_LOAD,
      .store_op = SDL_GPU_STOREOP_STORE,
      .texture = texture,
  };
  auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color, 1u, NULL);
  FML_DEFER(SDL_EndGPURenderPass(render_pass));

  ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
}

void Renderer::StartupIMGUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();
  ImGui_ImplSDLGPU3_InitInfo info = {
      .Device = context_->GetDevice().get(),
      .ColorTargetFormat = context_->GetColorFormat(),
      .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
  };
  FML_CHECK(ImGui_ImplSDL3_InitForSDLGPU(context_->GetWindow().get()));
  FML_CHECK(ImGui_ImplSDLGPU3_Init(&info));
}

void Renderer::ShutdownIMGUI() {
  ImGui_ImplSDL3_Shutdown();
  ImGui_ImplSDLGPU3_Shutdown();
  ImGui::DestroyContext();
}

SDL_GPUTexture* Renderer::RenderOnce() {
  const auto& device = context_->GetDevice();
  auto command_buffer = SDL_AcquireGPUCommandBuffer(device.get());
  if (!command_buffer) {
    FML_LOG(ERROR) << "Could not get command buffer: " << SDL_GetError();
    return NULL;
  }
  FML_DEFER(SDL_SubmitGPUCommandBuffer(command_buffer));
  SDL_GPUTexture* swapchain_image = nullptr;
  Uint32 texture_width = 0u;
  Uint32 texture_height = 0u;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer,               //
                                             context_->GetWindow().get(),  //
                                             &swapchain_image,             //
                                             &texture_width,               //
                                             &texture_height               //
                                             )) {
    FML_LOG(ERROR) << "Could not acquire swapchain image: " << SDL_GetError();
    return NULL;
  }
  if (swapchain_image == NULL) {
    // The wait completed with failure.
    FML_LOG(ERROR) << "Acquired swapchain texture was invalid.";
    return NULL;
  }

  const auto texture_format = context_->GetColorFormat();

  auto color_texture = CreateGPUTexture(device.get(),                         //
                                        {texture_width, texture_height, 1u},  //
                                        SDL_GPU_TEXTURETYPE_2D,               //
                                        texture_format,                       //
                                        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,    //
                                        1u,                                   //
                                        SDL_GPU_SAMPLECOUNT_4                 //
  );

  auto depth_texture =
      CreateGPUTexture(device.get(),                               //
                       {texture_width, texture_height, 1u},        //
                       SDL_GPU_TEXTURETYPE_2D,                     //
                       SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT,    //
                       SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,  //
                       1u,                                         //
                       SDL_GPU_SAMPLECOUNT_4                       //
      );

  if (!color_texture.IsValid() || !depth_texture.IsValid()) {
    return NULL;
  }

  const auto color_info = SDL_GPUColorTargetInfo{
      .texture = color_texture.texture.get().value,
      .resolve_texture = swapchain_image,
      .clear_color = {1.0f, 0.0f, 1.0f, 1.0f},
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_RESOLVE,
  };

  const auto depth_stencil_info = SDL_GPUDepthStencilTargetInfo{
      .texture = depth_texture.texture.get().value,
      .clear_depth = 0.0f,
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_DONT_CARE,
      .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
      .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
      .clear_stencil = 0,
  };

  auto render_pass = SDL_BeginGPURenderPass(command_buffer,      //
                                            &color_info,         //
                                            1,                   //
                                            &depth_stencil_info  //
  );

  if (!render_pass) {
    FML_LOG(ERROR) << "Could not begin render pass: " << SDL_GetError();
    return NULL;
  }
  FML_DEFER(SDL_EndGPURenderPass(render_pass));

  DrawContext context = {
      .command_buffer = command_buffer,
      .pass = render_pass,
      .viewport = {texture_width, texture_height},
  };

  for (auto& drawable : drawables_) {
    if (!drawable->Draw(context)) {
      return NULL;
    }
  }

  return swapchain_image;
}

}  // namespace ts
