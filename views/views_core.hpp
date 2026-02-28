#pragma once

#include "../util/util.hpp"
#include "../geometry/geometry.hpp"
#include <functional>

namespace weave {

template <class Range>
float max_text_width(Range& range, graphics_context& ctx, int font_size) {
  float res = 0;
  for (auto& s : range) 
    res = std::max( ctx.text_bounds(s, font_size).x, res );
  return res;
}

namespace impl {
  template <class... Args>
  struct widget_action {
    using type = std::function<void(event_context& ec, Args...)>;
  };
  
  template <class Rt, class... Args>
  struct widget_action<Rt(Args...)> {
    using type = std::function<Rt(event_context& ec, Args...)>;
  };
}

template <class... Args>
using widget_action = typename impl::widget_action<Args...>::type;

template <class T>
using write_fn = widget_action<void(T)>;

template <class State, class Fn, class... Args>
decltype(auto) context_invoke(Fn fn, event_context& ec, Args&&... args) {
  ec.request_rebuild();
  ec.request_repaint();
  if constexpr ( requires { std::invoke(fn, ec.template state<State>(), (Args&&)args...); } )
    return (std::invoke(fn, ec.template state<State>(), (Args&&)args...));
  else if constexpr ( requires { std::invoke(fn, ec, (Args&&)args...); } )
    return (std::invoke(fn, ec, (Args&&)args...)); 
  else
    return (std::invoke(fn, (Args&&)args...));
}

/// A simple helper for the observation of mutations of large data structure (images/array/ect)
template <class T>
struct observed_value {
  
  auto&& get(this auto&& self) { return self.value; }
  
  template <class V>
  auto& operator=(V&& v) {
    value = WEAVE_FWD(v);
    mut();
    return *this;
  }
  
  T* operator->() { return &value; }
  const T* operator->() const { return &value; }
  
  auto& mut() { ++version_count; return value; }
  
  unsigned version() const { return version_count; }
  void note_mutation() { ++version_count; }
  
  
  T value;
  private : 
  unsigned version_count = 0;
};

} // weave