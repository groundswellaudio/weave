#pragma once

#include "views_core.hpp"
#include <string_view>
#include "../cursor.hpp"

struct scrollable_widget {
  
  using value_type = void;
  
  float pos = 0;
  bool scrollbar = false;
  
  void on(mouse_event e, event_context<void> ec) {
    /* 
    if (e.is_mouse_scroll()) {
      pos += e.mouse_scroll_delta().x;
      scrollbar = true;
      ec.children()[0]->set_position(0, -pos);
    }
    else
      scrollbar = false; */ 
  }
  
  void on_child_event(input_event e, widget_id id, event_context<void> ec) 
  {
    if (e.is_mouse_scroll()) {
      pos += e.mouse_scroll_delta().x;
      scrollbar = true;
      ec.children()[0].set_position(0, -pos);
    }
    if (e.is_mouse_drag() && e.position.x > ec.this_size().x - 15)
      ;;
  }
  
  void paint(painter& p, vec2f this_size) {
    if (scrollbar)
      p.fill_rounded_rect({this_size.x - 15, 0}, {15, this_size.y}, 6);
  }
};

template <class View>
struct scrollable {
  
  template <class State>
  void construct(widget_tree_builder& b, State& state) {
    b.create_widget<scrollable_widget>( empty_lens{}, size );
  }
  
  template <class State>
  void rebuild(scrollable<View>& New, widget_tree_updater& up, State& s) {
    child.rebuild(New.child, up, s);
  }
  
  vec2f size;
  View child;
};

template <class V>
scrollable(vec2f sz, V view) -> scrollable<V>;