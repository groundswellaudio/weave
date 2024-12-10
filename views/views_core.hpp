#pragma once

#include "../util/util.hpp"

template <class Range>
float max_text_width(Range& range, graphics_context& ctx, int font_size = 11) {
  float res;
  for (auto& s : range) 
    res = std::max( ctx.text_bounds(s, font_size).x, res );
  return res;
}