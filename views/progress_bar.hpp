#pragma once

#include "views_core.hpp"

struct progress_bar_widget : widget_base
{
  float ratio;
  
  void on(ignore, ignore) 
  {
  }
  
  void paint(painter& p) {
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6, 1);
    p.fill_style(rgba_f{colors::cyan}.with_alpha(0.3));
    p.fill_rounded_rect({0, 0}, {size().x * ratio, size().y}, 6);
  }
};

namespace views {

struct progress_bar : view<progress_bar> {
  
  progress_bar(float ratio) : ratio{ratio} {}
  
  auto build(auto&& b, ignore) {
    return progress_bar_widget{{200, 15}, ratio};
  }
  
  rebuild_result rebuild(progress_bar Old, widget_ref w, ignore, ignore) {
    w.as<progress_bar_widget>().ratio = ratio;
    return {};
  }
  
  void destroy(widget_ref w) {}
  
  float ratio;
};

static_assert( is_view_sequence<progress_bar> );

}