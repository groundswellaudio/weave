#pragma once

#include "views_core.hpp"

namespace weave::widgets {

template <class Shape>
struct shape {
  
  Shape shape;
};

struct rectangle : widget_base {
  
  auto size_info() const {
    widget_size_info res;
    res.flex_factor = point{1, 1};
    res.nominal = nominal_size;
    return res;
  }
  
  void paint(painter& p) {
    if (stroke) {
      p.stroke_style(color);
      p.stroke(area(), stroke_width);
    }
    else {
      p.fill_style(color);
      p.fill(area());
    }
  }
  
  void on(mouse_event e, event_context& ec) {}
  
  point nominal_size;
  rgba_u8 color;
  short stroke_width;
  bool stroke = false;
};

} // widgets

namespace weave::views {
  
  struct rectangle : view<rectangle>, view_modifiers {
    
    auto build(ignore, ignore) {
      auto res = widgets::rectangle{};
      res.nominal_size = point{100, 30};
      res.color = color;
      res.stroke_width = stroke_w;
      res.stroke = do_stroke;
      return res;
    }
    
    rebuild_result rebuild(rectangle rect, widget_ref wr, ignore, ignore) {
      auto res = rebuild_result{};
      auto& w = wr.as<widgets::rectangle>();
      w.color = color;
      w.stroke_width = stroke_w;
      w.stroke = do_stroke;
      return {};
    }
    
    auto stroke(float w) {
      stroke_w = w;
      do_stroke = true;
      return *this;
    }
    
    rgba_u8 color;
    short stroke_w = 1;
    bool do_stroke = false;
  };
  
} // views