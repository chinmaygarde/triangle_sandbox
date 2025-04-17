#include "model_renderer.h"

#include "imgui.h"

namespace ts {

static const char* kModelCatalog[] = {"DamagedHelmet",
                                      "TextureCoordinateTest",
                                      "Duck",
                                      "BoomBox",
                                      "BarramundiFish",
                                      "Avocado",
                                      "CommercialRefrigerator",
                                      "DragonAttenuation"};

ModelRenderer::ModelRenderer(std::shared_ptr<Context> ctx)
    : context_(std::move(ctx)) {
  LoadModel(kModelCatalog[0]);
}

ModelRenderer::~ModelRenderer() {}

bool ModelRenderer::Draw(const DrawContext& context) {
  if (!is_valid_) {
    return false;
  }
  if (!model_) {
    return true;
  }

  static int current_model_index = 0;
  ImGui::ListBox("Model", &current_model_index, kModelCatalog,
                 IM_ARRAYSIZE(kModelCatalog));
  LoadModel(kModelCatalog[current_model_index]);

  return model_->Draw(context);
}

void ModelRenderer::LoadModel(const std::string& model_name) {
  if (is_valid_ && model_name_ == model_name) {
    return;
  }
  is_valid_ = false;
  auto model_data = fml::FileMapping::CreateReadOnly(fml::paths::JoinPaths(
      {MODELS_LOCATION, model_name, "glTF-Binary", model_name + ".glb"}));
  if (!model_data) {
    FML_LOG(ERROR) << "Could not load model data.";
    return;
  }
  auto model = std::make_unique<Model>(*context_, *model_data);

  if (!model->IsValid()) {
    FML_LOG(ERROR) << "Could not load model.";
    return;
  }

  model_ = std::move(model);
  model_name_ = model_name;
  is_valid_ = true;
}

}  // namespace ts
