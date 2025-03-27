#pragma once

#include <fml/logging.h>
#include "context.h"

namespace ts {

class Renderer {
 public:
  Renderer(std::shared_ptr<Context> context);

  bool Render();

 private:
  std::shared_ptr<Context> context_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Renderer);
};

}  // namespace ts
