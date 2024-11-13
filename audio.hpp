#pragma once
#include <SDL.h>

enum class mouse_cursor : char
{
  arrow      = SDL_SYSTEM_CURSOR_ARROW,
  ibeam      = SDL_SYSTEM_CURSOR_IBEAM,
  wait       = SDL_SYSTEM_CURSOR_WAIT,
  crosshair  = SDL_SYSTEM_CURSOR_CROSSHAIR,
  wait_arrow = SDL_SYSTEM_CURSOR_WAITARROW,
  size_nw_se = SDL_SYSTEM_CURSOR_SIZENWSE,
  size_ne_sw = SDL_SYSTEM_CURSOR_SIZENESW,
  size_we    = SDL_SYSTEM_CURSOR_SIZEWE,
  size_ns    = SDL_SYSTEM_CURSOR_SIZENS,
  size_all   = SDL_SYSTEM_CURSOR_SIZEALL,
  stop       = SDL_SYSTEM_CURSOR_NO,
  hand       = SDL_SYSTEM_CURSOR_HAND
};

inline void set_mouse_cursor(mouse_cursor cursor_kind = mouse_cursor::arrow)
{
  static SDL_Cursor* cursor {nullptr};

  if (cursor)
    SDL_FreeCursor(cursor);

  cursor = SDL_CreateSystemCursor( (SDL_SystemCursor)cursor_kind );

  SDL_SetCursor(cursor);
}