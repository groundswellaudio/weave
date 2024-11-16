#pragma once

#include "views_core.hpp"
#include <string_view>

/* 
struct text_widget
{
  std::string_view text;
  rgba_f color;
  
  using value_type = void;
  
  void on(ignore, ignore) 
  {
  }
  
  void paint(painter& p, vec2f this_size) {
    p.fill_style(rgba_f{1.f, 1.f, 0.f, 1.f});
    p.text_alignment(text_align::x::left, text_align::y::top);
    p.text({0, 0}, text);
  }
};

struct text : view {
  
  text(std::string_view str) : str{str} {}
  
  auto construct(widget_tree_builder& b, ignore) {
    return make_tuple( text_wigdet{text}, empty_lens{} );
    
    return b.template create_widget<text_widget>(empty_lens{}, {100, 15}, str);
  }
  
  void rebuild(text New, widget_tree_updater& b, ignore) {  
    auto& w = b.consume_leaf().as<text_widget>();
    if (*this == New)
      return;
    *this = New;
  }
  
  std::string_view str;
  float font_size = 13;
}; */ 