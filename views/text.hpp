#pragma once

#include "views_core.hpp"
#include <string_view>

struct text_widget : widget_base
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
    p.text_alignment(text_align::x::left, text_align::y::center);
    p.text({0, size().y / 2}, prop.text);
  }
};

struct text : view<text> {
  
  text(std::string_view str) : prop{str} {}
  
  auto build(auto&& b, ignore) {
    return text_widget{{{60, 30}, {0, 0}}, prop};
  }
  
  void rebuild(text Old, widget_ref w, auto& up, ignore) {
    if (prop != Old.prop)
      w.as<text_widget>().prop = prop;
  }
  
  void destroy(widget_ref w) {}
  
  text_widget::properties prop;
};