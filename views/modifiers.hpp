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

template <class W>
struct with_fixed_size : W {
  
  widget_size_info size_info() const {
    widget_size_info res;
    res.min = W::size();
    res.max = W::size();
    res.nominal_size = W::size();
    res.flex_factor = point{0, 0};
    return res;
  }
  
};

} // widgets

namespace weave::views {

template <class V, class Filter, class Action>
struct event_listener : V {
  using widget_t = widgets::event_listener<typename V::widget_t>;
  
  event_listener(auto&& v, Filter filter, Action action) 
    : V{(decltype(v)&&)v}, filter{filter}, action{action} {}
  
  template <class State>
  auto build(const build_context& b, State& state) {
    return widget_t{V::build(b, state), [action = action, filter = filter] (event_context& ec, input_event e) {
      auto mapped_event = std::invoke(filter, e);
      if (!mapped_event)
        return false;
      context_invoke<State>(action, ec, *mapped_event);
      return true;
    }};
  }
  
  auto rebuild(auto& old, widget_ref r, auto&& ctx, auto& state) {
    return V::rebuild(old, widget_ref{&r.as<widget_t>().base()}, ctx, state);
  }
  
  Filter filter;
  Action action;
};

template <class V, class Filter, class Fn>
event_listener(V, Filter filter, Fn fn) -> event_listener<V, Filter, Fn>;

inline bool is_mouse_down(input_event e) {
  return e.is_mouse() && e.mouse().is_down();
}

template <class V>
struct with_fixed_size : V {
  
  using widget_t = widgets::with_fixed_size<typename V::widget_t>; 
  
  auto build(const build_context& ctx, auto& state) {
    auto res = widget_t{ V::build(ctx, state) };
    res.set_size(fixed_size);
    return res;
  }
  
  rebuild_result rebuild(const with_fixed_size<V>& Old, widget_ref r, const build_context& ctx, auto& state) {
    auto& wb = r.as<widget_t>();
    V::rebuild(Old, widget_ref{&static_cast<typename V::widget_t&>(wb)}, ctx, state);
    if (fixed_size != Old.fixed_size) { 
      wb.set_size(fixed_size);
      return rebuild_result::size_change;
    }
    return {};
  }
   
  point fixed_size;
};

/// Common extensions for views.
struct view_modifiers {

  private : 
  
  struct one_keydown_filter {
    std::optional<keyboard_event> operator()(input_event e) const { 
      if (e.is_keyboard() && e.keyboard().is_down() && e.keyboard().key() == key)
        return e.keyboard();
      return {};
    }
    keycode key;
  };
  
  static std::optional<keyboard_event> keydown_filter(input_event e) {
    if (e.is_keyboard() && e.keyboard().is_down())
      return e.keyboard();
    return {};
  }
  
  static std::optional<std::string> file_drop_filter(input_event e) {
    if (e.is_mouse() && e.mouse().is_file_drop())
      return e.mouse().dropped_file();
    return {};
  }
  
  static std::optional<mouse_event> click_filter(input_event e) {
    if (e.is_mouse() && e.mouse().is_down())
      return e.mouse();
    return {};
  }
  
  public : 
  
  auto on_click(this auto&& self, auto action) {
    return event_listener{self, &click_filter, action};
  }
  
  auto on_key_down(this auto&& self, auto action) {
    return event_listener{self, &keydown_filter, action};
  }
  
  auto on_key_down(this auto&& self, keycode k, auto action) {
    return event_listener{self, one_keydown_filter{k}, action};
  }
  
  auto on_file_drop(this auto&& self, auto action) {
    return event_listener{self, &file_drop_filter, action};
  }
  
  auto with_fixed_size(this auto&& self, point size) {
    return views::with_fixed_size{self, size};
  }
};

} // views