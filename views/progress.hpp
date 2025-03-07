#pragma once

#include "views_core.hpp"

namespace weave::widgets {

struct progress_bar : widget_base
{
  float ratio;
  
  void on(ignore, ignore) {}
  
  vec2f min_size() const { return {80, 8}; }
  vec2f max_size() const { return {infinity<float>(), 8}; };
  
  void paint(painter& p) {
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6, 1);
    p.fill_style(rgba_f{colors::cyan}.with_alpha(0.3));
    p.fill_rounded_rect({0, 0}, {size().x * ratio, size().y}, 6);
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