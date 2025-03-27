#pragma once

#include <fml/unique_object.h>
#include "sdl_types.h"

namespace ts {

class Context {
 public:
  Context(UniqueSDLWindow window);

  const UniqueSDLWindow& GetWindow() const;

  const UniqueGPUDevice& GetDevice() const;

 private:
  UniqueSDLWindow window_;
  UniqueGPUDevice device_;

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Context);
};

}  // namespace ts
