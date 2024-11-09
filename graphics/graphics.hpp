#pragma once

#include "color.hpp"
#include "nanovg.h"

#define NANOVG_GL3_IMPLEMENTATION

#include "nanovg_gl.h"

namespace impl
{
  inline auto to_nvg_col(const rgba_f& c) { return nvgRGBAf(c[0], c[1], c[2], c[3]); }
} // impl

struct painter
{
  using color = rgba_f;
  
  void begin_frame(vec2f size, int ratio){
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    nvgBeginFrame(ctx, size.x, size.y, ratio);
  }

  void end_frame(){
    nvgEndFrame(ctx);
    glDisable(GL_STENCIL_TEST);
  }
  
  auto& fill_style(const color& c) {
    nvgFillColor(ctx, impl::to_nvg_col(c));
    return *this;
  }

  void rectangle(vec2f position, vec2f size) {
    position += origin;
    nvgBeginPath(ctx);
    nvgRect(ctx, position.x, position.y, size.x, size.y);
    nvgFill(ctx);
  }
  
  void rounded_rectangle(vec2f position, vec2f size, float radius) {
    position += origin;
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, position.x, position.y, size.x, size.y, radius);
    nvgFill(ctx);
  }
  
  void circle(vec2f position, float sz) {
    position += origin;
    nvgCircle(ctx, position.x, position.y, sz);
  }
  
  void translate(vec2f delta) {
    origin += delta;
  }
  
  void set_origin(vec2f origin_) {
    origin = origin_;
  }

  NVGcontext* ctx;
  vec2f origin {0, 0};
};

struct graphics_context 
{
  graphics_context()
  {}
  
  void init() 
  {
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
    {
      fprintf(stderr, "failed to initialize the OpenGL context.");
      exit(1);
    }
    
    ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    
    glClearColor(0,0,0,1);
	  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
  }
  
  ::painter painter() { return {ctx}; }
  
  NVGcontext* ctx = nullptr;
};