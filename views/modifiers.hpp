#pragma once

#include "views_core.hpp"
#include <functional>

namespace weave::widgets {

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

namespace weave::views {

template <class V, class Filter, class Action>
struct event_listener : view<on_click<V, Filter, Action>> {
  using widget_t = widgets::event_listener<typename V::widget_t>;
  
  event_listener(auto&& v, Filter filter, Action action) : view{v}, filter{filter}, action{action} {}
  
  template <class State>
  auto build(const widget_builder& b, State& state) {
    return widget_t{view.build(b, state), [action = action, filter = filter] (event_context& ec, input_event e) {
      return std::invoke(filter, e) && (context_invoke<State>(action, ec), true);
    }};
  }
  
  auto rebuild(auto& old, widget_ref r, auto&& ctx, auto& state) {
    return view.rebuild(old.view, widget_ref{&r.as<widget_t>().base()}, ctx, state);
  }
  
  V view;
  Filter filter;
  Action action;
};

template <class V, class Fn>
event_listener(V, Fn fn) -> on_click<V, Fn>;

inline bool is_mouse_down(input_event e) {
  return e.is_mouse_event() && e.mouse().is_mouse_down();
}

inline bool is_key_down(input_event e) {
  return e.is_keyboard() && e.keyboard().is_key_down();
}

struct with_event_modifiers {

  struct is_this_key_down {
    bool operator()(input_event e) const { return is_key_down(e) && e.keyboard().key == key; }
    keycode key;
  };
  
  auto on_click(this auto&& self, auto action) {
    return event_listener{self, &is_mouse_down, action};
  }
  
  auto on_key_down(this auto&& self, auto action) {
    return event_listener{self, &is_key_down, action};
  }
  
  auto on_key_down(this auto&& self, keycode k, auto action) {
    return event_listener{self, is_this_key_down{k}, action};
  }
};

} // views