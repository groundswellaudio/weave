#pragma once

#include "color.hpp"
#include "nanovg.h"

#define NANOVG_GL3_IMPLEMENTATION

#include "nanovg_gl.h"

namespace impl
{
  inline auto to_nvg_col(const rgba_f& c) { return nvgRGBAf(c[0], c[1], c[2], c[3]); }
} // impl

namespace text_align
{
	enum class x : int
	{
		right   = NVG_ALIGN_RIGHT,
		center  = NVG_ALIGN_CENTER,
		left    = NVG_ALIGN_LEFT
	};
	
	enum class y : int
	{
		top     = NVG_ALIGN_TOP,
		center  = NVG_ALIGN_MIDDLE,
		bottom  = NVG_ALIGN_BOTTOM
	};
}

struct painter_state
{
  painter_state(NVGcontext* backend_ptr) noexcept
  : ctx{backend_ptr}
  {
  }

  void text_alignment(text_align::x alignx, text_align::y aligny = text_align::y::center)
  {
    current_aligment = (int)(alignx) | (int)(aligny);
    nvgTextAlign(ctx, current_aligment);
  }

  // return the text bounding box if it were drawn at the given position
  // using the current alignment
  vec2f text_bounds(std::string_view str) const
  {
    float bounds[4];
    nvgTextBounds(ctx, 0, 0, str.data(), str.end(), bounds);
    return vec2f{ bounds[2] - bounds[0], bounds[3] - bounds[1] };
  }
	
  /* 
  auto make_image_from_file(const char* str)
  {
    auto res = nvgCreateImage(ctx, str, 0);
    if (res == 0)
      throw 1;
    return image{ctx, res};
  } */ 
	
  auto font_size() const {
    return current_font_size;
  }

  // Set the current font size
  void font_size(float size){
    current_font_size = size;
    nvgFontSize(ctx, size);
    update_font_offset();
  }

  void set_font(const char* ident){
    nvgFontFace(ctx, "default");
    update_font_offset();
  }
	
  /* 
  // Compute the glyph positions for @text at position @pos using the current aligment.
  void get_glyph_positions(glyph_positions& p, std::string_view text, screen_pt pos) const
  {
    p.positions.resize(text.size());
    nvgTextGlyphPositions( ctx, pos.x, pos.y, text.begin(), text.end(), p.positions.data(), (int)(p.positions.size()) );
  }

  auto nvg_lin_grad(const linear_gradient& grad) const {
    return nvgLinearGradient(ctx,
      grad.from_point.x, grad.from_point.y, grad.to_point.x, grad.to_point.y,
      impl::to_nvg_col(grad.from_col), impl::to_nvg_col(grad.to_col));
  } */ 

  auto nvg_ctx_internal_handle__() const {
    return ctx;
  }

  protected :

  void update_font_offset()
  {
    // due to font format unconsistency,
    // sometimes the ascender actually means "height max"
    // so we have to apply an offset to correct that if it's the case
    // ideally this should be handled by the renderer!
    float a, d, h;
    nvgTextMetrics(ctx, &a, &d, &h);
    text_vert_offset = (a > h) ? a - h : 0;
  }

  NVGcontext* ctx;
  int current_aligment;
  float current_font_size = 11;
  float text_vert_offset = 0;
};

struct painter : painter_state
{
  using color = rgba_f;
  
  void begin_frame(vec2f size, int ratio){
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    nvgBeginFrame(ctx, size.x, size.y, ratio);
  }
  
  void scissor(vec2f pos, vec2f size){
    nvgScissor(ctx, pos.x, pos.y, size.x, size.y);
  }
  
  void intersect_scissor(vec2f pos, vec2f size) {
    nvgIntersectScissor(ctx, pos.x, pos.y, size.x, size.y);
  }

  void reset_scissor() {
    nvgResetScissor(ctx);
  }
	
  void begin_path() {
    nvgBeginPath(ctx);
  }

  void close_path() {
    nvgClosePath(ctx);
  }
	
  void end_frame(){
    nvgEndFrame(ctx);
    glDisable(GL_STENCIL_TEST);
  }
  
  void stroke_rect(vec2f pos, vec2f size, float thick = 1) {
    nvgBeginPath(ctx);
    nvgRect(ctx, pos.x, pos.y, size.x, size.y);
    nvgStrokeWidth(ctx, thick);
    nvgStroke(ctx);
  }
  
  void stroke_rounded_rect(vec2f pos, vec2f size, float rounding_radius, float thickness = 1) {
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, pos.x, pos.y, size.x, size.y, rounding_radius);
    nvgStrokeWidth(ctx, thickness);
    nvgStroke(ctx);
  }

  void fill_rounded_rect(vec2f pos, vec2f size, float rounding_radius) {
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, pos.x, pos.y, size.x, size.y, rounding_radius);
    nvgFill(ctx);
  }

  void stroke_style(const color& c) {
    nvgStrokeColor(ctx, impl::to_nvg_col(c));
  }

  auto& fill_style(const color& c) {
    nvgFillColor(ctx, impl::to_nvg_col(c));
    return *this;
  }

  void rectangle(vec2f position, vec2f size) {
    nvgBeginPath(ctx);
    nvgRect(ctx, position.x, position.y, size.x, size.y);
    nvgFill(ctx);
  }
  
  void rounded_rectangle(vec2f position, vec2f size, float radius) {
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, position.x, position.y, size.x, size.y, radius);
    nvgFill(ctx);
  }
  
  void circle(vec2f position, float sz) {
    nvgBeginPath(ctx);
    nvgCircle(ctx, position.x, position.y, sz);
    nvgFill(ctx);
  }
  
  void text(vec2f pos, std::string_view v) {
    nvgText(ctx, origin.x + pos.x, origin.y + pos.y - this->text_vert_offset, v.data(), v.end());
  }
  
  void translate(vec2f delta) {
    nvgTranslate(ctx, delta.x, delta.y);
  }
  
  void set_origin(vec2f origin_) {
    origin = origin_;
  }
  
  void push_transform() {
    nvgSave(ctx);
  }
  
  void pop_transform() {
    nvgRestore(ctx);
  }

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
    
    auto fontId = nvgCreateFont(ctx, "default", "/Users/groundswell/Desktop/spiral/spiral/swansea.ttf");
    
    glClearColor(0,0,0,1);
	  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
  }
  
  ::painter painter() { return {ctx}; }
  
  NVGcontext* ctx = nullptr;
};