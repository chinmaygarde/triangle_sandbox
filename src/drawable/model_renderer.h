#pragma once

#include <fml/mapping.h>
#include <fml/paths.h>
#include "drawable.h"
#include "model.h"
#include "models_location.h"

namespace ts {

class ModelRenderer final : public Drawable {
 public:
  ModelRenderer(const UniqueGPUDevice& device, const std::string& model_name) {
    auto model_data = fml::FileMapping::CreateReadOnly(fml::paths::JoinPaths(
        {MODELS_LOCATION, model_name, "glTF-Binary", model_name + ".glb"}));
    if (!model_data) {
      FML_LOG(ERROR) << "Could not load model data.";
      return;
    }
    Model model(device, *model_data);
    if (!model.IsValid()) {
      FML_LOG(ERROR) << "Could not load model.";
      return;
    }
    is_valid_ = true;
  }

  bool Draw(SDL_GPURenderPass* pass) override { return true; }

 private:
  bool is_valid_ = false;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(ModelRenderer);
};

}  // namespace ts
