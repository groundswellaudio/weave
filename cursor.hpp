#pragma once

namespace weave {

#include <SDL3/SDL.h>

enum class mouse_cursor : char
{
  system_default = SDL_SYSTEM_CURSOR_DEFAULT,
  text      = SDL_SYSTEM_CURSOR_TEXT,
  wait       = SDL_SYSTEM_CURSOR_WAIT,
  crosshair  = SDL_SYSTEM_CURSOR_CROSSHAIR,
  wait_arrow = SDL_SYSTEM_CURSOR_PROGRESS,
  size_nw_se = SDL_SYSTEM_CURSOR_NWSE_RESIZE,
  size_ne_sw = SDL_SYSTEM_CURSOR_NESW_RESIZE,
  size_we    = SDL_SYSTEM_CURSOR_EW_RESIZE,
  size_ns    = SDL_SYSTEM_CURSOR_NS_RESIZE,
  size_all   = SDL_SYSTEM_CURSOR_MOVE,
  stop       = SDL_SYSTEM_CURSOR_NOT_ALLOWED,
  hand       = SDL_SYSTEM_CURSOR_POINTER
};

inline void set_mouse_cursor(mouse_cursor cursor_kind = mouse_cursor::system_default)
{
  static SDL_Cursor* cursor {nullptr};

  if (cursor)
    SDL_DestroyCursor(cursor);

  cursor = SDL_CreateSystemCursor( (SDL_SystemCursor)cursor_kind );

  SDL_SetCursor(cursor);
}

} // weave