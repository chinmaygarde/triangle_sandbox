#pragma once

#include <fml/mapping.h>
#include <fml/paths.h>
#include "drawable.h"
#include "model.h"
#include "models_location.h"

namespace ts {

class ModelRenderer final : public Drawable {
 public:
  ModelRenderer(const Context& ctx, const std::string& model_name);

  ~ModelRenderer();

  bool Draw(const DrawContext& context) override;

 private:
  std::unique_ptr<Model> model_;
  bool is_valid_ = false;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(ModelRenderer);
};

}  // namespace ts
