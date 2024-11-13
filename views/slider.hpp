#pragma once

#include "views_core.hpp"
#include <format>

struct slider_x_node 
{
  using value_type = float;
  
  void on(mouse_event e, vec2f this_size, event_context<float> ec) 
  {
    if (!e.is_mouse_drag() && !e.is_mouse_down())
      return;
    
    auto sz = this_size;
    float ratio = std::min(e.position.x / sz.x, 1.f);
    ratio = std::max(0.f, ratio);
    
    ec.write(ratio);
    //ec.repaint_request();
  }
  
  void paint(painter& p, float pc, vec2f this_size) {
    auto sz = this_size;
    p.fill_style(rgb_u8{30, 30, 30});
    p.fill_rounded_rect({0, 0}, sz, 6);
    p.fill_style(rgba_f{colors::cyan}.with_alpha(0.3));
    auto e = pc;
    p.rectangle({0, 0}, {e * sz.x, sz.y});
    
    p.stroke_style(rgba_f{colors::white}.with_alpha(0.5));
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
    auto next = b.template create_widget<slider_x_node>(lens, size);
  }
  
  template <class S>
  void rebuild(slider<Lens> New, widget_tree_updater& b, S& state) {
    b.consume_leaf();
    *this = New;
  }
  
  /* 
  void destroy(widget_tree& t) {
    //t.destroy(view::this_id);
  } */ 
  
  Lens lens;
  vec2f size = {80, 15};
};

template <class Lens>
slider(Lens) -> slider<Lens>;