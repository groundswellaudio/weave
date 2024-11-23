#pragma once
#include <SDL.h>

struct window {
  
  window(const char* name, vec2f size) {
    init(name, size.x, size.y);
  }
  
  //window() : win{nullptr}, gl_ctx{nullptr} {}
  
  window(window&& w) noexcept {
    win    = std::exchange(w.win,  nullptr);
    gl_ctx = std::exchange(gl_ctx, nullptr);
  }
  
  ~window(){
    if (!win) return;
    SDL_DestroyWindow(win);
    SDL_GL_DeleteContext(gl_ctx);
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
  
  /* 
  void min_size(screen_pt sz) {
    SDL_SetWindowMinimumSize(win, sz.x, sz.y);
  }
  
  void max_size(screen_pt sz) {
    SDL_SetWindowMaximumSize(win, sz.x, sz.y);
  } */ 
  
  void swap_buffer() {
    SDL_GL_SwapWindow(win);
  }
  
  private :
  
  void init(const char* name, int x, int y) {
    win    = SDL_CreateWindow(name, 0, 0, x, y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    gl_ctx = SDL_GL_CreateContext(win);
  }
  
  SDL_Window* win;
  void* gl_ctx;
};