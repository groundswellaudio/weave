#pragma once

#include "views_core.hpp"
#include <string_view>
#include "../cursor.hpp"

namespace weave::widgets {

struct scrollable_base {
  
  static constexpr float bar_width = 8.f;
  
  // requires three member functions : 
  // scroll_zone, which is the rectangle of the scrollable area, 
  // scroll_size, the total size of the scroll
  // and displace_scroll(delta)
  
  rectangle scrollbar_rect(this auto& self) {
    auto scrollzone = self.scroll_zone().size.y;
    auto ratio = std::min( scrollzone / self.scroll_size(), 1.f );
    return {{self.size().x - bar_width, self.scroll_zone().origin.y + self.scrollbar_pos}, 
            {bar_width, ratio * scrollzone}};
  }
  
  void scrollbar_move(this auto& self, float drag_delta, event_context& ec) {
    auto old_bar_pos = self.scrollbar_pos;
    self.scrollbar_pos = std::clamp(self.scrollbar_pos + drag_delta, 0.f, 
                                    self.scroll_zone().size.y - self.scrollbar_rect().size.y);
    if (self.scrollbar_pos != old_bar_pos)
    {
      drag_delta = self.scrollbar_pos - old_bar_pos;
      auto scrollable_delta = self.scroll_size() * drag_delta / self.scroll_zone().size.y;
      self.displace_scroll(scrollable_delta);
      ec.request_repaint();
    }
  }
  
  bool on(this auto& self, mouse_event e, event_context& ec) {
    if (e.is_scroll()) 
      return (self.scrollbar_move(e.scroll_delta().y, ec), true);
    else if (e.is_drag() && self.is_dragging) 
      return (self.scrollbar_move(e.drag_delta().y, ec), true);
    else if (self.scrollbar_rect().contains(e.position) && e.is_down())
      return (self.is_dragging = true, true);
    self.is_dragging = false;
    return false;
  }
  
  void on_child_event(this auto& self, mouse_event e, event_context& ec) {
    if (e.is_scroll())
      self.scrollbar_move(e.scroll_delta(), ec);
  }
  
  void paint(this auto& self, painter& p) {
    p.fill_style(colors::gray);
    auto r = self.scrollbar_rect();
    if (r.size.y != self.scroll_zone().size.y)
      p.fill(rounded_rectangle(r));
  }
  
  private : 
  
  float scrollbar_pos = 0;
  bool is_dragging = false;
};

template <class T>
struct scrollable : widget_base, scrollable_base {
  
  rectangle scroll_zone() const {
    return {{0, 0}, size()};
  }
  
  float scroll_size() const {
    return child.size().y;
  }
  
  auto traverse_children(auto fn) {
    return fn(child);
  }
  
  auto layout() {
    child.layout();
    return size();
  }
  
  void displace_scroll(float delta) {
    child.set_position(child.position() - vec2f{0, delta});
  }
  
  T child; 
};

} // widgets

namespace weave::views {

template <class View>
struct scrollable : view<scrollable<View>> {
  
  using widget_t = widgets::scrollable<typename View::widget_t>;
  
  scrollable(vec2f sz, View child) : size{sz}, child{child} {}
  
  template <class State>
  auto build(build_context b, State& state) {
    return widget_t{ {size}, {}, child.build(b, state) };
  }
  
  template <class State>
  void rebuild(scrollable<View>& New, widget_ref w, build_context up, State& s) {
    child.rebuild(New.child, widget_ref{&w.as<widget_t>().child}, up, s);
  }
  
  vec2f size;
  View child;
};

template <class V>
scrollable(vec2f sz, V view) -> scrollable<V>;

} // views