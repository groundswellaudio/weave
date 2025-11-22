#pragma once

#include "views_core.hpp"

namespace weave::widgets {

struct progress_bar : widget_base
{
  float ratio;
  
  void on(ignore, ignore) {}
  
  widget_size_info size_info() const {
    vec2<size_policy> policy {
      {size_policy::losslessly_shrinkable, size_policy::expansion_neutral},
      {size_policy::losslessly_shrinkable, size_policy::not_expansible}
    };
    widget_size_info res {policy};
    res.min = point{50, 10};
    res.nominal_size = point{100, 15};
    res.max.y = 15;
    return res;
  }
  
  void paint(painter& p) {
    p.stroke_style(colors::white);
    p.stroke(rounded_rectangle(size()));
    p.fill_style(rgba_f{colors::cyan}.with_alpha(0.3));
    p.fill(rounded_rectangle({size().x * ratio, size().y}));
  }
};

} // widgets

namespace weave::views {

struct progress_bar : view<progress_bar> {
  
  using widget_t = widgets::progress_bar;
  
  progress_bar(float ratio) : ratio{ratio} {}
  
  auto build(auto&& b, ignore) {
    return widget_t{{200, 15}, ratio};
  }
  
  rebuild_result rebuild(progress_bar Old, widget_ref w, ignore, ignore) {
    w.as<widget_t>().ratio = ratio;
    return {};
  }
  
  void destroy(widget_ref w) {}
  
  float ratio;
};

}