#pragma once

#include "../util/util.hpp"
#include <functional>

template <class Range>
float max_text_width(Range& range, graphics_context& ctx, int font_size = 11) {
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