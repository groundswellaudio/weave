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

struct slider_x_widget : widget_base
{
  slider_properties prop;
  std::string value_str;
  
  using value_type = float;
  
  using EvCtx = event_context<slider_x_widget>;
  
  void on_value_change(float new_val) {
    value_str = std::format("{:.5}", new_val);
  }
  
  void on(mouse_event e, EvCtx ec) 
  {
    if (!e.is_mouse_drag() && !e.is_mouse_down())
      return;
    
    auto sz = size();
    float ratio = std::min(e.position.x / sz.x, 1.f);
    ratio = std::max(0.f, ratio);
    
    auto new_val = prop.min + ratio * (prop.max - prop.min); 
    ec.write(new_val);
    on_value_change(new_val);
    //ec.repaint_request();
  }
  
  void paint(painter& p, float pc) 
  {
    auto sz = size();
    p.fill_style(prop.background_color);
    p.fill_rounded_rect({0, 0}, sz, 6);
    p.fill_style(prop.active_color);
    auto e = (pc - prop.min) / (prop.max - prop.min);
    p.rectangle({0, 0}, {e * sz.x, sz.y});
    
    static constexpr auto outline_col = rgba_f{colors::white}.with_alpha(0.5);
    p.stroke_style(outline_col);
    p.stroke_rounded_rect({0, 0}, sz, 6, 1);
    
    p.fill_style(colors::white);
    p.text_alignment(text_align::x::center, text_align::y::center);
    p.font_size(11);
    p.text({sz.x / 2, sz.y / 2}, value_str);
  }
};

template <class Lens>
struct slider : view<slider<Lens>> {
  
  slider(Lens L) : lens{L} {}
  
  template <class S>
  auto build(const widget_builder& b, S& state) {
    val = lens(state);
    auto res = slider_x_widget{{size}, properties};
    res.on_value_change(val);
    return with_lens<S>(std::move(res), lens);
  }
  
  template <class S>
  rebuild_result rebuild(slider<Lens>& Old, widget_ref wb, widget_updater up, S& state) {
    auto& w = wb.as<slider_x_widget>();
    val = lens(state);
    if (properties != Old.properties) {
      w.prop = properties;
    }
    if (val != Old.val) {
      w.on_value_change(val);
    }
    if (size != Old.size) {
      w.set_size(size);
      return rebuild_result{true};
    }
    return {};
  }
  
  auto& with_range(float min, float max) {
    properties.min = min;
    properties.max = max;
    return *this;
  }
  
  void destroy(widget_ref w) {}
  
  slider_properties properties;
  Lens lens;
  float val;
  vec2f size = {80, 15};
};

template <class Lens>
slider(Lens) -> slider<Lens>;