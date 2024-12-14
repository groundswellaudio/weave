#pragma once

#include "views_core.hpp"
#include <functional>

namespace widgets {

template <class W>
struct event_listener : W {
  
  widget_action<bool(input_event)> fn; 
  
  using base_t = W;
  
  auto& base() { return (W&)*this; }
  
  void on(mouse_event e, event_context& ec) {
    if (fn(ec, e))
      return;
    base().on(e, ec);
  }
  
  void on(keyboard_event e, event_context& ec) {
    if (fn(ec, e))
      return;
    if constexpr ( requires {base().on(e, ec);} ) 
      base().on(e, ec);
  }
};

} // widgets

namespace views {

template <class V, class Action>
struct on_click : view<on_click<V, Action>> {
  using widget_t = widgets::event_listener<typename V::widget_t>;
  
  on_click(auto&& v, auto&& action) : view{v}, action{action} {}
  
  template <class State>
  auto build(const widget_builder& b, State& state) {
    return widget_t{view.build(b, state), [action = action] (event_context& ec, input_event e) {
      if (!e.is_mouse_event() || !e.mouse().is_mouse_down())
        return false;
      context_invoke<State>(action, ec);
      return true;
    }};
  }
  
  auto rebuild(auto& old, widget_ref r, auto&& ctx, auto& state) {
    return view.rebuild(old.view, widget_ref{&r.as<widget_t>().base()}, ctx, state);
  }
  
  V view;
  Action action;
};

template <class V, class Fn>
on_click(V, Fn fn) -> on_click<V, Fn>;

} // views