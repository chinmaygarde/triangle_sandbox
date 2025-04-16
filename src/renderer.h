#pragma once

#include <fml/logging.h>
#include "context.h"
#include "drawable.h"

namespace ts {

class Renderer {
 public:
  Renderer(std::shared_ptr<Context> context);

  ~Renderer();

  bool Render();

 private:
  std::shared_ptr<Context> context_;
  std::vector<std::unique_ptr<Drawable>> drawables_;

  void StartupIMGUI();

  void ShutdownIMGUI();

  void BeginIMGUIFrame();

  void EndIMGUIFrame(SDL_GPUTexture* texture);

  SDL_GPUTexture* RenderOnce();

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(Renderer);
};

}  // namespace ts
