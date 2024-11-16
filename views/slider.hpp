#pragma once

#include "views_core.hpp"
#include <format>

struct slider_properties {
  bool operator==(const slider_properties&) const = default;
  float min = 0, max = 1;
  
  rgba_u8 background_color = rgb_u8{30, 30, 30};
  rgba_u8 active_color = rgba_f{colors::cyan}.with_alpha(0.3);
  rgba_u8 text_color = colors::white;
};

struct slider_x_widget 
{
  slider_properties prop;
  float ratio;
  
  using value_type = float;
  
  void on(mouse_event e, event_context<float> ec) 
  {
    if (!e.is_mouse_drag() && !e.is_mouse_down())
      return;
    
    auto sz = ec.this_size();
    ratio = std::min(e.position.x / sz.x, 1.f);
    ratio = std::max(0.f, ratio);
    
    auto new_val = prop.min + ratio * (prop.max - prop.min); 
    ec.write(new_val);
    //ec.repaint_request();
  }
  
  void paint(painter& p, float pc, vec2f this_size) 
  {
    auto sz = this_size;
    p.fill_style(prop.background_color);
    p.fill_rounded_rect({0, 0}, sz, 6);
    p.fill_style(prop.active_color);
    auto e = ratio;
    p.rectangle({0, 0}, {e * sz.x, sz.y});
    
    static constexpr auto outline_col = rgba_f{colors::white}.with_alpha(0.5);
    p.stroke_style(outline_col);
    p.stroke_rounded_rect({0, 0}, this_size, 6, 1);
    
    p.fill_style(colors::white);
    p.text_alignment(text_align::x::center, text_align::y::center);
    auto str = std::format("{}", pc);
    p.font_size(11);
    p.text({this_size.x / 2, this_size.y / 2}, str);
  }
};

template <class Lens>
struct slider {
  
  slider(Lens L) : lens{L} {}
  
  template <class S>
  auto construct(widget_tree_builder& b, S& state) {
    auto next = b.template create_widget<slider_x_widget>(lens, size, properties);
  }
  
  template <class S>
  void rebuild(slider<Lens> New, widget_tree_updater& b, S& state) {
    auto& w = b.consume_leaf().as<slider_x_widget>();
    if (w.prop != New.properties)
      w.prop = New.properties; 
    *this = New;
  }
  
  auto& with_range(float min, float max) {
    properties.min = min;
    properties.max = max;
    return *this;
  }
  
  /* 
  void destroy(widget_tree& t) {
    //t.destroy(view::this_id);
  } */ 
  
  slider_properties properties;
  Lens lens;
  vec2f size = {80, 15};
};

template <class Lens>
slider(Lens) -> slider<Lens>;