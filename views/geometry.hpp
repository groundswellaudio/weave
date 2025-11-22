#pragma once

#include "views_core.hpp"

namespace weave::widgets {

template <class Shape>
struct shape {
  
  Shape shape;
};

template <>
struct shape<rectangle> : widget_base {
  
  widget_size_info size_info() const {
    size_policy sp {size_policy::losslessly_shrinkable, size_policy::expansion_neutral};
    widget_size_info res {{sp, sp}};
    res.nominal_size = nominal_size;
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
  
  point nominal_size;
  rgba_u8 color;
  short stroke_width;
  bool stroke = false;
};

} // widgets

namespace weave::views {
  
  struct rectangle : view<rectangle> {
    
    rectangle(vec2f sz, rgba_u8 col) : sz{sz}, color{col} {}
    rectangle(geo::rectangle_f r, rgba_u8 col) {
      sz = r.size;
      origin = r.origin;
      color = col;
    }
    
    auto build(ignore, ignore) {
      auto res = widget_t{sz, color, stroke_w, do_stroke};
      if (origin)
        res.set_position(*origin);
      return res;
    }
    
    auto rebuild(ignore, widget_ref wr, ignore, ignore) {
      auto res = rebuild_result{};
      auto& w = wr.as<widget_t>();
      if (w.size() != rect.size) {
        w.set_size(rect.size);
        res |= rebuild_result::size_changed;
      }
      if (origin)
        w.set_position(origin);
      w.color = color;
      w.stroke_width = stroke_w;
      w.stroke = do_stroke;
      return w;
    }
    
    rgba_u8 color;
    std::optional<vec2f> origin; 
    vec2f sz;
    short stroke_w = 1;
    bool do_stroke = false;
  };
  
} // views