#include "glad/glad.h"
#include "graphics.hpp"

#define NANOVG_GL3_IMPLEMENTATION

#include "nanovg_gl.h"
#include <SDL.h>

#include "misc/default_font_embed"

graphics_context::graphics_context()
{
  if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
  {
    fprintf(stderr, "failed to initialize the OpenGL context.");
    exit(1);
  }
  
  ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
  
  create_font_from_memory("default", default_font_data);
  
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}