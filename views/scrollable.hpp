#pragma once

#include "views_core.hpp"
#include <string_view>
#include "../cursor.hpp"

struct scrollable_widget {
  
  using value_type = void;
  
  static constexpr auto bar_width = 8;
  
  widget* child;
  float scrollbar_ratio = 0, scrollbar_start = 0;
  
  bool scrollbar = false;
  
  void on(mouse_event e, event_context<void> ec) {
    
  }
  
  vec2f layout(vec2f sz) {
    auto res = child->layout();
    scrollbar_ratio = std::max(sz.y / res.y, 1.f);
    return {res.x + bar_width, sz.y};
  }
  
  void on_child_event(input_event e, event_context<void> ec, widget_id id) 
  {
    if (e.is_mouse_scroll()) {
      scrollbar = true;
      auto new_y = std::clamp(child->position().y + e.mouse_scroll_delta().y, -child->size().y, 0.f);
      
      child->set_position(0, new_y);
      scrollbar_start = -child->position().y / child->size().y;
      scrollbar_ratio = ec.this_size().y / child->size().y;
    }
    if (e.is_mouse_drag() && e.position.x > ec.this_size().x - bar_width) {
      scrollbar_start += e.mouse_drag_delta().y / ec.this_size().x;
      child->set_position( 0, -scrollbar_start * child->size().y );
    }
      ;;
  }
  
  void paint(painter& p, vec2f sz) 
  {
    p.fill_style(colors::white);
    p.fill_rounded_rect({sz.x - bar_width, 0}, {bar_width, sz.y}, 6);
    p.fill_style(colors::gray);
    auto p2 = {sz.x - bar_width, scrollbar_ratio * sz.y};
    p.fill_rounded_rect({sz.x - bar_width, scrollbar_start * sz.y}, 
                        {bar_width, sz.y * scrollbar_ratio}, 6);
  }
  
  std::span<widget*> children() {
    return {&child, 1};
  }

  widget* find_child_at(vec2f pos) {
    return child;
  }
};

static_assert( is_child_event_listener<scrollable_widget, event_context<void>> );

template <class View>
struct scrollable : view<scrollable<View>> {
  
  scrollable(vec2f sz, View child) : size{sz}, child{child} {}
  
  template <class State>
  auto build(widget_builder b, State& state) {
    auto this_id = b.next_id();
    auto build_res = child.build(b, state);
    auto child = b.make_widget(this_id, build_res);
    return make_tuple( scrollable_widget{child}, empty_lens{}, widget_ctor_args{this_id, size} );
  }
  
  template <class State>
  void rebuild(scrollable<View>& New, widget& w, widget_updater up, State& s) {
    child.rebuild(New.child, *w.as<scrollable_widget>().child, up, s);
  }
  
  vec2f size;
  View child;
};

template <class V>
scrollable(vec2f sz, V view) -> scrollable<V>;  