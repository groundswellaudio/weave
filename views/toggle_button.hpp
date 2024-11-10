#pragma once

#include "views_core.hpp"
#include <string_view>

struct toggle_button_widget
{
  static constexpr float button_radius = 8;
  
  std::string_view str;
  
  using value_type = bool;
  
  void on(mouse_event e, vec2f sz, event_context<bool> ec) {
    if (e.is_mouse_down())
      ec.write(!ec.read());
  }
  
  void paint(painter& p, bool flag, vec2f sz) 
  {
    p.fill_style(rgba_f{1.f, 1.f, 0.f, 1.f});
    
    p.circle({button_radius, button_radius}, button_radius);
    
    if (flag)
    {
      p.fill_style(colors::red);
      p.circle({button_radius, button_radius}, 6);
    }
    
    p.fill_style(colors::white);
    p.text_alignment(text_align::x::left, text_align::y::center);
    p.font_size(11);
    p.text( {button_radius * 2 + 2, button_radius}, str );
  }
};

template <class Lens>
struct toggle_button : view {
  
  toggle_button(Lens, std::string_view str) : str{str} {}
  
  auto construct(widget_tree_builder& b) {
    return b.create_widget<toggle_button_widget, Lens>( view::view_id, {60, 15}, str);
  }
  
  std::string_view str;
  float font_size = 13;
};

/* 
template <class L>
struct toggle_button(L) -> toggle_button<L>; */ 