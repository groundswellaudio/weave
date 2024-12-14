#pragma once

#include "views_core.hpp"
#include <string_view>

namespace widgets {

struct text : widget_base
{
  struct properties {
    bool operator==(const properties& o) const = default;
    std::string_view text;
    rgba_u8 color = colors::white;
    float font_size = 11;
  };
  
  properties prop;
  
  using value_type = void;
  
  void on(ignore, ignore) 
  {
  }
  
  void paint(painter& p) {
    p.font_size(prop.font_size);
    p.fill_style(prop.color);
    p.text_align(text_align::x::left, text_align::y::center);
    p.text({0, size().y / 2}, prop.text);
  }
};

} // widgets

namespace views {

struct text : view<text> {
  
  text(std::string_view str) : prop{str} {}
  text(text&&) = default;
  text(const text&) = default;
  
  vec2f bounds(graphics_context& gctx) {
    return prop.text.size() == 0 ? vec2f{0, prop.font_size} : gctx.text_bounds(prop.text, prop.font_size);
  }
  
  using widget_t = widgets::text;
  
  auto build(auto&& b, ignore) {
    auto& gctx = b.context().graphics_context();
    return widget_t{{bounds(gctx)}, prop};
  }
  
  rebuild_result rebuild(text Old, widget_ref w, auto& up, ignore) {
    auto& wb = w.as<widget_t>();
    rebuild_result res {};
    if (prop.text != wb.prop.text) {
      auto new_bounds = bounds(up.context().graphics_context());
      wb.set_size(new_bounds);
      wb.prop.text = prop.text;
      res |= rebuild_result::size_change;
    }
    return res;
  }
  
  void destroy(widget_ref w) {}
  
  widget_t::properties prop;
};

} // views