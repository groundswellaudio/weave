#pragma once

#include "color.hpp"
#include "image.hpp"
#include "../geometry/geometry.hpp"

#include "util/iota.hpp"
#include "util/util.hpp"
#include "util/vec.hpp"

#include "nanovg.h"

#include <cassert>
#include <optional>
#include <vector>
#include <string>
#include <string_view>

namespace weave {

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

struct rounded_rectangle : rectangle {
  
  rounded_rectangle(point size, float r = 6) : rounded_rectangle(rectangle(size), r) {}
  
  rounded_rectangle(const rectangle& rect, float r = 6) : rectangle(rect), rounding{r, r, r, r}
  {
  }
  
  rounded_rectangle(const rectangle& rect, const std::array<float, 4>& rounding) 
  : rectangle{rect}, rounding{rounding}
  {
  }
   
  std::array<float, 4> rounding;
};

struct painter
{
  using color = rgba_f;
  
  NVGcontext* ctx;
  float text_vert_offset = 0;
  int current_alignment = 0;
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
  
  struct scissor_raii {
    painter& p; 
    ~scissor_raii() { 
      p.reset_scissor();
    }
  };
  
  [[nodiscard]] scissor_raii scissor(point pos, point size){
    nvgScissor(ctx, pos.x, pos.y, size.x, size.y);
    return {*this};
  }
  
  void intersect_scissor(point pos, point size) {
    nvgIntersectScissor(ctx, pos.x, pos.y, size.x, size.y);
  }

  void reset_scissor() {
    nvgResetScissor(ctx);
  }
  
  void text_align(text_align::x alignx, text_align::y aligny = text_align::y::center)
  {
    current_alignment = (int)(alignx) | (int)(aligny);
    nvgTextAlign(ctx, current_alignment);
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
  
  auto& path(circle c) {
    nvgCircle(ctx, c.center.x, c.center.y, c.radius);
    return *this;
  }
  
  auto& path(const triangle& t) {
    move_to(t.a).line_to(t.b).line_to(t.c);
    return *this;
  }
  
  auto& path(const rectangle& r) {
    nvgRect(ctx, r.origin.x, r.origin.y, r.size.x, r.size.y);
    return *this;
  }
  
  auto& path(const rounded_rectangle& r) {
    nvgRoundedRectVarying(ctx, r.origin.x, r.origin.y, r.size.x, r.size.y, 
                          r.rounding[0], r.rounding[1], r.rounding[2], r.rounding[3]);
    return *this;
  }
  
  painter& move_to(point p) {
    nvgMoveTo(ctx, p.x, p.y);
    return *this;
  }

  painter& line_to(point p) {
    nvgLineTo(ctx, p.x, p.y);
    return *this;
  }
  
  void fill_path() {
    nvgFill(ctx);
  }
  
  void stroke_path(float thickness) {
    nvgStrokeWidth(ctx, thickness);
    nvgStroke(ctx);
  }
  
  template <class T>
  void fill(const T& shape) {
    begin_path().path(shape).fill_path();
  }
  
  template <class T>
  void stroke(const T& shape, float thickness = 1) {
    begin_path().path(shape).stroke_path(thickness);
  }
  
  void line(vec2f a, vec2f b, float thick = 3) {
    begin_path();
    move_to(a);
    line_to(b);
    nvgStrokeWidth(ctx, thick);
    nvgStroke(ctx);
  }

  void stroke_style(const color& c) {
    nvgStrokeColor(ctx, impl::to_nvg_col(c));
  }

  void fill_style(const color& c) {
    nvgFillColor(ctx, impl::to_nvg_col(c));
  }
  
  void fill_style(texture_handle t, point top_left, point size) {
    auto p = nvgImagePattern(ctx, top_left.x, top_left.y, size.x, size.y, 0, t.id, 1.f);
    nvgFillPaint(ctx, p);
  }
  
  void text(point pos, std::string_view v) {
    nvgText(ctx, pos.x, pos.y - this->text_vert_offset, v.data(), v.end());
  }
  
  void text_bounded(point pos, float width, std::string_view str) {
    if (str == "")
      return;
    float bounds[4];
    std::vector<NVGglyphPosition> positions;
    positions.resize(str.size());
    nvgTextGlyphPositions( ctx, pos.x, pos.y, str.begin(), str.end(), positions.data(), (int)(positions.size()) );
    
    if (positions.back().maxx - positions.front().minx > width) {
      // Look at the space needed to display ... 
      /* 
      NVGglyphPosition ellipsisPos[3];
      nvgTextGlyphPositions(ctx, 0, 0, "...", nullptr, ellipsisPos, 3);
      auto ellipsis_width = ellipsisPos[2].maxx; */ 
      
      nvgSave(ctx);
      
      // apply the scissor horizontally
      intersect_scissor({pos.x, pos.y - (float)1e6}, {width, 2 * 1e6});
      
      auto ellipsis_width = font_size();
      
      auto it = std::find_if( positions.rbegin(), positions.rend(), 
        [&] (auto& e) { return e.maxx + ellipsis_width < positions.front().minx + width; } );
      
      int index = it == positions.rend() ? 0 : (int) str.size() - (it - positions.rbegin());
      
      text( pos, std::string_view{str.begin(), str.begin() + index} );
      
      // FIXME : Take into account top alignment
      auto ellipsis_y = (current_alignment | (int) text_align::y::center) 
        ? (pos.y + font_size() / 2 - ellipsis_width / 10)
        : pos.y; 
        
      auto c = circle(vec2f{positions[0].minx + width, ellipsis_y}, ellipsis_width / 10);
      c = c.translated({-c.radius * 2 - 3, 0});
      
      fill( c );
      fill( c.translated({-ellipsis_width / 3, 0}) );
      fill( c.translated({-2 * ellipsis_width / 3, 0}) );
      
      nvgRestore(ctx);
      // text( vec2f{positions[index].minx, pos.y}, "..." );
      
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
  
  [[nodiscard]] translation_raii translate(point delta) {
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

struct glyph_positions {
    
  std::size_t size() const { return positions.size(); }
  auto min() const { return positions.front().minx; }
  auto max() const { return positions.back().maxx;  }
  
  float pos_from_index(unsigned idx) const {
    if (idx == size())
      return max();
    return positions[idx].x;
  }
  
  unsigned index_from_pos(float pos) const {
    if (pos < min())
      return 0;
    if (pos > max())
      return (unsigned)positions.size();
    for (auto& p : positions)
      if (pos >= p.minx && pos <= p.maxx)
        return (unsigned)(&p - positions.data());
    return (unsigned)positions.size();
  }
  
  std::vector<NVGglyphPosition> positions;
};

struct graphics_context 
{
  graphics_context();
  
  void get_glyph_positions(glyph_positions& p, std::string_view text, point pos) const {
    p.positions.resize(text.size());
    nvgTextGlyphPositions(ctx, pos.x, pos.y, text.begin(), text.end(), p.positions.data(), (int)(p.positions.size()) );
  }

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
  
  painter painter() { return {ctx, text_vert_offset}; }
  
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

} // weave