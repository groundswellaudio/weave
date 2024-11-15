#pragma once

#include "views_core.hpp"
#include <string_view>
#include "../cursor.hpp"

struct scrollable_widget {
  
  float pos;
  bool scrollbar = false;
  
  void on(mouse_event e, event_context<void> ec) {
    else if (e.is_mouse_scroll()) {
      pos += e.mouse_scroll_delta().x;
      scrollbar = true;
      ec.children()[0]->set_position(0, -pos);
    }
    else
      scrollbar = false;
  }
  
  void on_child_event(input_event e, event_context<void> ec, widget_id id) {
    if (e.is_mouse_scroll())
  }
  
  void paint(painter& p, vec2f this_size) {
    if (scrollbar)
      p.fill_rounded_rect({this_size - 8, pos}, {8, scroll_end});
  }
};

template <class View>
struct scrollable {
  
  template <class State>
  void build(widget_tree_builder& b, State& state) {
    b.create_widget<scrollable_widget>( empty_lens{},  
  }
  
  template <class State>
  void rebuild(scrollable<View>& New, widget_tree_updater& up, State& s) {
    child.rebuild(New.child, up, s);
  }
  
  vec2f size;
  View child;
};

/*
template <class L>
struct toggle_button(L) -> toggle_button<L>; */ 