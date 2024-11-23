#pragma once

#include "views_core.hpp"
#include "../util/tuple.hpp"

#include <cmath>

struct numeric_field_properties {
  float min, max;
};

struct numeric_field_widget : widget_base 
{
  using prop_t = numeric_field_properties;
  
  numeric_field_widget(prop_t prop) : prop{prop} {}
  
  numeric_field_properties prop;
  float mult_mod;
  
  using Self = numeric_field_widget;
  using value_type = float;
  
  void on(mouse_event e, event_context<Self>& ec) {
    if (e.is_mouse_down())
      mult_mod = (1.f - 3 * e.position.x / size().x);
    else if (e.is_mouse_drag())
    {
      auto delta = - std::pow(10, mult_mod) * e.mouse_drag_delta().y;
      auto NewVal = std::clamp<float>( ec.read() + delta, prop.min, prop.max );
      ec.write( NewVal );
    }
  }
  
  void paint(painter& p, float value) {
    p.stroke_style(colors::red);
    p.stroke_rect({0, 0}, size());
    p.font_size(13.f);
    auto str = std::format("{:.2f}", value);
    p.text_alignment(text_align::x::center, text_align::y::center);
    p.text( sz / 2, str );
  }
  
  auto layout() const { return vec2f{50, 30}; }
};

template <class L>
using numeric_field = simple_view_for<numeric_field_widget, L, numeric_field_properties>;