#pragma once

#include "views_core.hpp"

#include <vector>

namespace weave::widgets {

struct zstack : widget_base {
  
  std::vector<element> children;
  
  auto traverse_children(auto&& fn) {
    return std::all_of(children.begin(), children.end(), fn);
  }
  
  void on(input_event e, event_context& ec) {}
};

struct zstack_with_lasso : widget_base {
  
  struct element {
    widget_box widget;
    ptr<void(widget_base*, bool)> set_selected;
  };
  std::vector<element> children;
  std::optional<rectangle> lasso;
  
  auto traverse_children(auto&& fn) {
    return std::all_of(children.begin(), children.end(), 
                       [fn] (auto& elem) { return fn(elem.widget); } );
  }
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_down())
      lasso->set_origin(e.position);
    else if (e.is_drag())
    {
      lasso->set_corner(e.position);
      for (auto& c : children)
        if (c.widget.area().intersect(lasso))
          c.set_selected(true);
    }
    else if (e.is_up())
      lasso.reset();
  }
  
  void paint(painter& p) {
    if (lasso) {
      p.stroke_style(colors::white);
      p.stroke(*lasso, 1);
    }
  }
};

}