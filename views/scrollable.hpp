#pragma once

#include "views_core.hpp"
#include <string_view>
#include "../cursor.hpp"

struct scrollable_widget : widget_base {
  
  struct scrollbar : widget_base {
    
    using value_type = void;
    
    void on(mouse_event e, event_context<scrollbar> ec) 
    {
      if (e.is_mouse_drag()) 
        ec.parent().as<scrollable_widget>().scrollbar_move(e.mouse_drag_delta());
    }
    
    void paint(painter& p)
    {
      p.fill_style(colors::gray);
      p.fill_rounded_rect({0, 0}, size(), 6);
    }
  };

  using value_type = void;
  
  static constexpr auto bar_width = 8;
  
  widget_box child;
  scrollbar bar;
  
  float scrollbar_ratio = 0, scrollbar_start = 0;
  
  void on(mouse_event e, event_context<scrollable_widget> ec) {
    bar.on(e, ec);
  }
  
  vec2f layout() {
    auto res = child.layout();
    scrollbar_ratio = std::max(sz.y / res.y, 1.f);
    bar.set_position(res.x, 0);
    bar.set_size({bar_width, sz.y});
    return {res.x + bar_width, sz.y};
  }
  
  void traverse_children(auto fn) {
    fn(child);
    fn(bar);
  }
  
  void scrollbar_move(vec2f drag_delta) {
    scrollbar_start += drag_delta.y / size().y;
    child.set_position(0, -scrollbar_start * child.size().y);
    bar.set_position(child.size().x - bar_width, scrollbar_start * child.size().y);
  }
  
  void on_child_event(input_event e, event_context<scrollable_widget> ec, ignore) 
  {
    if (e.is_mouse_scroll()) {
      auto child_pos = child.position();
      auto child_sz = child.size();
      auto new_y = std::clamp(child_pos.y + e.mouse_scroll_delta().y, -child_sz.y, 0.f);
      
      child.set_position(0, new_y);
      scrollbar_start = -child_pos.y / child_sz.y;
      scrollbar_ratio = size().y / child_sz.y;
    }
    /* 
    if (e.is_mouse_drag() && e.position.x > size().x - bar_width) {
      scrollbar_start += e.mouse_drag_delta().y / ec.this_size().x;
      child->set_position( 0, -scrollbar_start * child->size().y );
    } */ 
  }
  
  void paint(painter& p) 
  {
    p.fill_style(colors::white);
    p.fill_rounded_rect({sz.x - bar_width, 0}, {bar_width, sz.y}, 6);
    p.fill_style(colors::gray);
    auto p2 = {sz.x - bar_width, scrollbar_ratio * sz.y};
    p.fill_rounded_rect({sz.x - bar_width, scrollbar_start * sz.y}, 
                        {bar_width, sz.y * scrollbar_ratio}, 6);
  }
};

static_assert( is_child_event_listener<scrollable_widget> );

template <class View>
struct scrollable : view<scrollable<View>> {
  
  scrollable(vec2f sz, View child) : size{sz}, child{child} {}
  
  template <class State>
  auto build(widget_builder b, State& state) {
    return scrollable_widget{ {size}, widget_box{child.build(b, state)} };
  }
  
  template <class State>
  void rebuild(scrollable<View>& New, widget_ref w, widget_updater up, State& s) {
    child.rebuild(New.child, w.as<scrollable_widget>().child.borrow(), up, s);
  }
  
  vec2f size;
  View child;
};

template <class V>
scrollable(vec2f sz, V view) -> scrollable<V>;  