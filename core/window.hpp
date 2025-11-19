#pragma once

#include <SDL3/SDL.h>
#include "../util/vec.hpp"

namespace weave {

struct window_properties {
  std::string name;
  vec2f size {600, 400};
};

struct window {
  
  window(window_properties prop) {
    init(prop.name.data(), prop.size.x, prop.size.y);
  }
  
  window(window&& w) noexcept {
    win    = std::exchange(w.win,  nullptr);
    gl_ctx = std::exchange(gl_ctx, nullptr);
  }
  
  ~window(){
    if (!win) return;
    SDL_DestroyWindow(win);
    SDL_GL_DestroyContext(gl_ctx);
  }
  
  SDL_Window* get(){ return win; }
  
  void maximize()        { SDL_MaximizeWindow(win);     }
  void minimize()        { SDL_MinimizeWindow(win);     }
  void restore()         { SDL_RestoreWindow(win);      }
  void hide()            { SDL_HideWindow(win);         }
  void show()            { SDL_ShowWindow(win);         }
  void rise()            { SDL_RaiseWindow(win);        }
  
  unsigned id() const    { return SDL_GetWindowID(win); }
  
  bool is_active() const { return win; }

  vec2f size() const {
    vec2i res;
    SDL_GetWindowSize(win, &res.x, &res.y);
    return {(float)res.x, (float)res.y};
  }
  
  vec2f position() const { return {0, 0}; }
  
  void set_min_size(vec2f sz) {
    SDL_SetWindowMinimumSize(win, sz.x, sz.y);
  }
  
  void set_max_size(vec2f sz) {
    SDL_SetWindowMaximumSize(win, sz.x, sz.y);
  }
  
  void swap_buffer() {
    SDL_GL_SwapWindow(win);
  }
  
  private :
  
  void init(const char* name, int x, int y) {
    win    = SDL_CreateWindow(name, x, y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    gl_ctx = SDL_GL_CreateContext(win);
  }
  
  SDL_Window* win;
  SDL_GLContext gl_ctx;
};

} // weave