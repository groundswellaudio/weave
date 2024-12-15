#pragma once

#include "color.hpp"
#include "nanovg.h"

#include "stb_image.h"
#include "stb_image_write.h"

#include <vec.hpp>

#include <cassert>
#include <optional>
#include <vector>
#include <string>
#include <initializer_list>

#include "util/iota.hpp"
#include "util/util.hpp"

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
  auto t = template_of(templ);
  auto args = arguments( (class_template_type) templ );
  template_arguments new_args {elem};
  for (auto a : args)
  for (int k = 1; k < size(args); ++k)
    push_back(new_args, args[k]);
  return instantiate( (class_template_decl) t, new_args);
}

template <class Pixel>
struct image {
  
  image(const image& o) = default;
  image() = default;
  image(vec2i size) {
    reshape(size);
  }
  
  template <class V>
  image(vec2i size, std::initializer_list<V> inits) 
  {
    for (auto i : inits)
      buffer.emplace_back(i);
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
  
  template <class P2>
  image<P2> to(this const auto& self) {
    image<P2> res;
    res.reshape(self.shape());
    for (int y = 0; y < self.shape()[0]; ++y)
      for (int x = 0; x < self.shape()[1]; ++x)
        res(y, x) = static_cast<P2>(self(y, x));
    return res;
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
  
  bool operator==(const image& o) const {
    if (shape() != o.shape())
      return false;
    for (int x0 : iota(shape()[0]))
      for (int x1 : iota(shape()[1]))
        if ((*this)({x0, x1}) != o({x0, x1}))
          return false;
    return true;
  }
  
  auto size() const { return buffer.size(); }
  
  std::vector<Pixel> buffer;
  vec2<int> shape_v {0, 0};
};

template <class Pixel, class T>
image<Pixel> make_image_from_raw(const T* ptr, vec2<int> sz) {
  image<Pixel> res;
  res.buffer.resize(sz.x * sz.y);
  res.shape_v = sz;
  std::memcpy(res.buffer.data(), ptr, res.buffer.size() * sizeof(Pixel));
  return res;
}

/// Decode a compressed image buffer
inline std::optional<image<rgba<unsigned char>>> decode_image(std::span<const unsigned char> data) {
  int w, h, n;
  auto img_data = stbi_load_from_memory(data.data(), data.size(), &w, &h, &n, 4);
  if (!img_data)
    return {};
  auto img = make_image_from_raw<rgba<unsigned char>>(img_data, {h, w});
  stbi_image_free(img_data);
  return img;
}

inline std::optional<image<rgba<unsigned char>>> load_image_from_file(const std::string& path) {
  int w, h, n;
  stbi_set_unpremultiply_on_load(1);
  stbi_convert_iphone_png_to_rgb(1);
  auto img = stbi_load(path.data(), &w, &h, &n, 4);
  if (!img) 
    return {};
  auto res = make_image_from_raw<rgba<unsigned char>>(img, {h, w});
  stbi_image_free(img);
  return res;
}

inline bool save_image_to_file(const std::string& path, const image<rgba<unsigned char>>& img) 
{
  auto w = img.shape()[1];
  auto h = img.shape()[0];
  
  if (path.ends_with(".jpg") || path.ends_with(".jpeg"))
    return stbi_write_jpg(path.c_str(), w, h, 4, img.data(), 80);
  
  return stbi_write_png(path.c_str(), w, h, 4, img.data(), w * 4); 
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

struct painter
{
  using color = rgba_f;
  
  NVGcontext* ctx;
  float text_vert_offset = 0;
  int current_aligment = 0;
  float current_font_size = 11;
  
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
  
  void scissor(vec2f pos, vec2f size){
    nvgScissor(ctx, pos.x, pos.y, size.x, size.y);
  }
  
  void intersect_scissor(vec2f pos, vec2f size) {
    nvgIntersectScissor(ctx, pos.x, pos.y, size.x, size.y);
  }

  void reset_scissor() {
    nvgResetScissor(ctx);
  }
  
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
  
  auto& begin_path() {
    nvgBeginPath(ctx);
    return *this;
  }
  
  auto& close_path() {
    nvgClosePath(ctx);
    return *this;
  }
  
  auto& move_to(vec2f p) {
    nvgMoveTo(ctx, p.x, p.y);
    return *this;
  }

  auto& line_to(vec2f p) {
    nvgLineTo(ctx, p.x, p.y);
    return *this;
  }
  
  void fill_path() {
    nvgFill(ctx);
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
  
  void triangle(vec2f a, vec2f b, vec2f c) {
    begin_path().move_to(a).line_to(b).line_to(c).close_path().fill_path();
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
  
  void text_bounded(vec2f pos, float width, std::string_view str) {
    if (str == "")
      return;
    float bounds[4];
    std::vector<NVGglyphPosition> positions;
    positions.resize(str.size());
    nvgTextGlyphPositions( ctx, pos.x, pos.y, str.begin(), str.end(), positions.data(), (int)(positions.size()) );
    
    if (positions.back().maxx > pos.x + width) {
      // Look at the space needed to display ... 
      NVGglyphPosition ellipsisPos[3];
      nvgTextGlyphPositions(ctx, 0, 0, "...", nullptr, ellipsisPos, 3);
      auto ellipsis_width = ellipsisPos[2].maxx;
      
      auto it = std::find_if( positions.rbegin(), positions.rend(), 
        [&] (auto& e) { return e.maxx + ellipsis_width < pos.x + width; } );
      
      int index = it == positions.rend() ? 0 : (int) str.size() - (it - positions.rbegin());
      
      text( pos, std::string_view{str.begin(), str.begin() + index} );
      text( vec2f{positions[index].minx, pos.y}, "..." );
      return;
    }
    
    text(pos, str);
  }
  
  struct translation_raii {
    painter& self;
    vec2f delta;
    ~translation_raii() {
      nvgTranslate(self.ctx, -delta.x, -delta.y);
    }
  };
  
  [[nodiscard]] translation_raii translate(vec2f delta) {
    nvgTranslate(ctx, delta.x, delta.y);
    return translation_raii{*this, delta}; 
  }
  
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
    auto id = nvgCreateImageRGBA(ctx, size[1], size[0], 0, data_ptr);
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
  
  ::painter painter() { return {ctx, text_vert_offset}; }
  
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