#include "model_renderer.h"

namespace ts {

ModelRenderer::ModelRenderer(const Context& ctx,
                             const std::string& model_name) {
  auto model_data = fml::FileMapping::CreateReadOnly(fml::paths::JoinPaths(
      {MODELS_LOCATION, model_name, "glTF-Binary", model_name + ".glb"}));
  if (!model_data) {
    FML_LOG(ERROR) << "Could not load model data.";
    return;
  }
  auto model = std::make_unique<Model>(ctx, *model_data);

  if (!model->IsValid()) {
    FML_LOG(ERROR) << "Could not load model.";
    return;
  }

  model_ = std::move(model);
  is_valid_ = true;
}

ModelRenderer::~ModelRenderer() {}

bool ModelRenderer::Draw(const DrawContext& context) {
  if (!model_) {
    return false;
  }
  return model_->Draw(context);
}

}  // namespace ts
