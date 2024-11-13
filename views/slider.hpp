#pragma once

#include "views_core.hpp"

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
    p.fill_style(rgba_f{1.f, 0.f, 0.f, 1.f});
    p.rectangle({0, 0}, sz);
    p.fill_style(colors::lime);
    auto e = pc;
    p.rectangle({0, 0}, {e * sz.x, sz.y});
  }
};

template <class Lens>
struct slider : view {
  
  slider(Lens L) : lens{L} {}
  
  template <class S>
  auto construct(widget_tree_builder& b, S& state) {
    auto next = b.template create_widget<slider_x_node>(lens, size);
    this_id = next.parent_widget()->id();
  }
  
  template <class S>
  void rebuild(slider<Lens> New, widget_tree_builder& b, S& state) {
    *this = New;
  }
  
  void destroy(widget_tree& t) {
    t.destroy(view::this_id);
  }
  
  Lens lens;
  vec2f size = {80, 15};
};

template <class Lens>
slider(Lens) -> slider<Lens>;