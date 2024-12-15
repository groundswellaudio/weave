#pragma once

#include "views_core.hpp"
#include <functional>

template <class T>
struct selection_value {
  
  auto setter(T val) { 
    return [this, val] (std::function<void()> next_on_change) { 
      value = val;
      if (on_change)
        on_change();
      on_change = next_on_change;
    };
  }
  
  T value;
  std::function<void()> on_change;
};

namespace widgets {

template <class W>
struct selectable : W {
  
  using base_t = W;
  
  auto& base() { return (W&)*this; }
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_mouse_down())
    {
      is_selected = true;
      on_select(ec, [this] () { this->unselect(); });
      //group_ptr->on_select(key, [this] () {unselect();});
    }
    W::on(e, ec);
  }
  
  void unselect() {
    is_selected = false;
  }
  
  void paint(painter& p) {
    W::paint(p);
    if (is_selected) {
      p.fill_style(rgba_f{colors::white}.with_alpha(0.3));
      p.rounded_rectangle({0, 0}, this->size(), 5);
    }
  }
  
  widget_action<std::function<void()>> on_select;
  bool is_selected = false;
};

} // widgets

namespace views {

template <class V, class OnSelect>
struct selectable : view<selectable<V, OnSelect>> {
  using widget_t = widgets::selectable<typename V::widget_t>;
  
  selectable(auto&& v, auto&& selection) : view{v}, on_select{selection} {}
  
  auto build(const widget_builder& b, auto& state) {
    auto res = widget_t{ view.build(b, state) };
    res.on_select = [f = on_select] (event_context& ec, auto next_on_change) { 
      ec.request_rebuild();
      f(next_on_change);
    };
    return res;
  }
  
  auto rebuild(auto& old, widget_ref r, auto&& ctx, auto& state) {
    return view.rebuild(old.view, widget_ref{&r.as<widget_t>().base()}, ctx, state);
  }
  
  V view;
  OnSelect on_select;
};

template <class V, class Fn>
selectable(V, Fn fn) -> selectable<V, Fn>;

} // views