#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <fml/logging.h>
#include <hedley.h>
#include "backends/imgui_impl_sdl3.h"
#include "renderer.h"

namespace ts {

static std::unique_ptr<Renderer> renderer_;

HEDLEY_C_DECL
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  SDL_SetAppMetadata("Triangle Sandbox", "1.0",
                     "com.chinmaygarde.triangle_sandbox");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  UniqueSDLWindow window(SDL_CreateWindow("Triangle Sandbox", 1200, 800, 0));

  if (!window.is_valid()) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_SetWindowResizable(window.get(), true);

  renderer_ =
      std::make_unique<Renderer>(std::make_unique<Context>(std::move(window)));
  return SDL_APP_CONTINUE;
}

HEDLEY_C_DECL
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }
  ImGui_ImplSDL3_ProcessEvent(event);
  return SDL_APP_CONTINUE;
}

HEDLEY_C_DECL
SDL_AppResult SDL_AppIterate(void* appstate) {
  if (renderer_) {
    return renderer_->Render() ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

HEDLEY_C_DECL
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  renderer_.reset();
}

}  // namespace ts
