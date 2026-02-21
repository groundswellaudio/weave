#pragma once

#include "views_core.hpp"

#include <vector>
#include <algorithm>

namespace weave::widgets {

struct zstack : widget_base {
  
  std::vector<widget_box> children;
  widget_size_info size_info_v; 
  
  auto traverse_children(auto&& fn) {
    return std::all_of(children.begin(), children.end(), fn);
  }
  
  // Override this so that the last children get the mouse event first
  optional<widget_ref> find_child_at(point pos) {
    auto it = std::find_if(children.rbegin(), children.rend(), 
                          [pos] (auto& w) { return w.contains(pos); });
    if (it == children.rend())
      return {};
    return it->borrow();
  }
  
  auto size_info() const {
    return size_info_v;
  }
  
  void on(input_event e, event_context& ec) {}
  
  void paint(painter& p) {}
};

} // widgets