#pragma once

#include <fml/mapping.h>
#include <fml/paths.h>
#include "drawable.h"
#include "model.h"
#include "models_location.h"

namespace ts {

class ModelRenderer final : public Drawable {
 public:
  ModelRenderer(std::shared_ptr<Context> ctx);

  ~ModelRenderer();

  bool Draw(const DrawContext& context) override;

 private:
  std::shared_ptr<Context> context_;
  std::unique_ptr<Model> model_;
  bool is_valid_ = false;
  std::string model_name_;

  void LoadModel(const std::string& model_name);

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(ModelRenderer);
};

}  // namespace ts
