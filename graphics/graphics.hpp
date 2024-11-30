#pragma once

#include "color.hpp"
#include "nanovg.h"

#include "stb_image.h"

#include <vec.hpp>

#include <cassert>
#include <optional>
#include <vector>
#include <string>

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

consteval type rebind(type templ, type elem) {
  auto t = template_of( (class_template_spec_decl) (class_template_type) templ);
  auto args = arguments( (class_template_type) templ );
  template_arguments new_args {elem};
  for (auto a : args)
    push_back(new_args, a);
  return instantiate(t, new_args);
}

template <class Pixel>
struct image {
  
  image() = default;
  image(vec2i size) {
    reshape(size);
  }
  
  using pixel_type = Pixel;
  
  template <class T>
  using pixel_template = %rebind(^Pixel, ^T);
  
  template <class T>
    requires (is_instance_of(remove_reference(^T), ^image))
  auto&& operator()(this T&& self, int y, int x) {
    assert( y >= 0 && y < self.shape()[0] && x >= 0 && x < self.shape()[1] );
    return self.buffer[y * self.shape()[1] + x];
  }
  
  template <class T>
    requires (is_instance_of(remove_reference(^T), ^image))
  auto&& operator() (this T&& self, vec2i pt) {
    return self(pt[0], pt[1]);
  }
  
  void reshape(vec2i new_shape) {
    buffer.resize(new_shape.x * new_shape.y);
    shape_v = new_shape;
  }
  
  bool empty() const { return buffer.empty(); }
  auto data() const { return buffer.data(); }
  auto shape() const { return shape_v; }
  
  auto begin(this auto& self) { return self.buffer.begin(); }
  auto end(this auto& self) { return self.buffer.end(); }
  
  std::vector<Pixel> buffer;
  vec2<int> shape_v {0, 0};
};

template <class Pixel, class T>
image<Pixel> make_image_from_raw(const T* ptr, vec2<int> sz) {
  image<Pixel> res;
  res.buffer.resize(sz.x * sz.y);
  res.shape_v = sz;
  std::memcpy(res.buffer.data(), ptr, res.buffer.size());
  return res;
}

inline std::optional<image<rgba<unsigned char>>> load_image_from_file(const std::string& path) {
  int w, h, n;
  stbi_set_unpremultiply_on_load(1);
  stbi_convert_iphone_png_to_rgb(1);
  auto img = stbi_load(path.data(), &w, &h, &n, 4);
  if (!img) 
    return {};
  auto res = make_image_from_raw<rgba<unsigned char>>(img, {w, h});
  stbi_image_free(img);
  return res;
}

struct graphics_context; 
struct painter;

struct texture_handle {
  
  friend graphics_context;
  friend painter;
  
  texture_handle(const texture_handle&) = default; 
  
  private :
  
  texture_handle(int v) : id{v} {}
  
  int id;
};

struct painter_state
{
  private : 
  
  friend graphics_context;

  painter_state(NVGcontext* backend_ptr, float text_y_offset) 
  : ctx{backend_ptr}, text_vert_offset{text_y_offset}
  {
  }
  
  public : 
  
  void text_align(text_align::x alignx, text_align::y aligny = text_align::y::center)
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
    nvgFontFace(ctx, ident);
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
  int current_aligment = 0;
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
  
  void move_to(vec2f p) {
    nvgMoveTo(ctx, p.x, p.y);
  }

  void line_to(vec2f p) {
    nvgLineTo(ctx, p.x, p.y);
  }
  
  void line(vec2f a, vec2f b, float thick = 3) {
    begin_path();
    move_to(a);
    line_to(b);
    nvgStrokeWidth(ctx, thick);
    nvgStroke(ctx);
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

  void fill_style(const color& c) {
    nvgFillColor(ctx, impl::to_nvg_col(c));
  }
  
  void fill_style(texture_handle t, vec2f top_left, vec2f size) {
    auto p = nvgImagePattern(ctx, top_left.x, top_left.y, size.x, size.y, 0, t.id, 1.f);
    nvgFillPaint(ctx, p);
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
  
  void stroke_circle(vec2f pos, float rad, float thick){
    nvgBeginPath(ctx);
    nvgCircle(ctx, pos.x, pos.y, rad);
    nvgStrokeWidth(ctx, thick);
    nvgStroke(ctx);
  }
  
  void circle(vec2f position, float sz) {
    nvgBeginPath(ctx);
    nvgCircle(ctx, position.x, position.y, sz);
    nvgFill(ctx);
  }
  
  void text(vec2f pos, std::string_view v) {
    nvgText(ctx, pos.x, pos.y - this->text_vert_offset, v.data(), v.end());
  }
  
  void translate(vec2f delta) {
    nvgTranslate(ctx, delta.x, delta.y);
  }
};

struct graphics_context 
{
  graphics_context();
  
  vec2f text_bounds(std::string_view str, int font_size) const {
    nvgFontSize(ctx, font_size);
    float bounds[4];
    nvgTextBounds(ctx, 0, 0, str.data(), str.end(), bounds);
    return vec2f{ bounds[2] - bounds[0], bounds[3] - bounds[1] };
  }
  
  texture_handle create_texture(const image<rgba<unsigned char>>& data, vec2<int> size) {
    auto data_ptr = reinterpret_cast<const unsigned char*>(data.data());
    auto id = nvgCreateImageRGBA(ctx, size.x, size.y, 0, data_ptr);
    return texture_handle{id};
  }
  
  void update_texture(texture_handle id, const unsigned char* data, vec2<int> size) {
    nvgUpdateImage(ctx, id.id, data);
  }
  
  void delete_texture(texture_handle id) {
    nvgDeleteImage(ctx, id.id);
  }
  
  void create_font_from_memory(std::string name, std::span<unsigned char> bytes) {
    nvgCreateFontMem(ctx, name.data(), bytes.data(), bytes.size(), 0);
  }

  void set_font(const std::string& ident){
    nvgFontFace(ctx, ident.c_str());
    update_font_offset();
  }
  
  ::painter painter() { return {{ctx, text_vert_offset}}; }
  
  private : 
  
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
  
  NVGcontext* ctx = nullptr;
  float text_vert_offset;
};