#pragma once

#include "views_core.hpp"
#include <string_view>

namespace widgets {

struct text : widget_base
{
  struct properties {
    bool operator==(const properties& o) const = default;
    rgba_u8 color = colors::white;
    float font_size = 11;
  };
  
  std::string str;
  properties prop;
  float text_width = 20;
  
  vec2f min_size() const { return {20, prop.font_size + 2}; }
  vec2f max_size() const { return {text_width + 20, prop.font_size + 2}; }
  
  void on(ignore, ignore) 
  {
  }
  
  void paint(painter& p) {
    p.font_size(prop.font_size);
    p.fill_style(prop.color);
    p.text_align(text_align::x::left, text_align::y::center);
    p.text({0, size().y / 2}, str);
  }
};

template <class W>
struct with_label : W {
  
  vec2f size() {
    return W::size() + vec2f{text_size, 0};
  }
  
  auto position() const {
    return W::position() - vec2f{text_size, 0};
  }
  
  void set_position(vec2f pos) {
    W::set_position( pos + vec2f{text_size, 0} );
  }
  
  void paint(painter& p) {
    p.font_size(11);
    p.fill_style(colors::white);
    p.text_align(text_align::x::left, text_align::y::center);
    p.text({0, W::size().y / 2}, text);
  }
  
  std::string text;
  float text_size;
};

} // widgets

namespace views {

struct text : view<text> {
  
  text(std::string_view str) : str{str} {}
  
  text(text&&) = default;
  text(const text&) = default;
  
  vec2f bounds(graphics_context& gctx) {
    return str.size() == 0 ? vec2f{0, prop.font_size} : gctx.text_bounds(str, prop.font_size);
  }
  
  using widget_t = widgets::text;
  
  auto build(auto&& b, ignore) {
    auto& gctx = b.context().graphics_context();
    auto box = bounds(gctx);
    return widget_t{{box}, std::string(str), prop, box.x};
  }
  
  rebuild_result rebuild(text Old, widget_ref w, auto& up, ignore) {
    auto& wb = w.as<widget_t>();
    rebuild_result res {};
    if (str != wb.str) {
      auto new_bounds = bounds(up.context().graphics_context());
      wb.set_size(new_bounds);
      wb.str = str;
      res |= rebuild_result::size_change;
    }
    return res;
  }
  
  void destroy(widget_ref w) {}
  
  widget_t::properties prop;
  std::string_view str;
};

template <class V>
struct with_label {
  
  auto build(auto&& b, auto& state) {
    return widgets::with_label{view.build(b, state), str};
  }
  
  V view;
  std::string_view str;
};

} // views