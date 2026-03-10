
#pragma once

#include "shapes.hpp"
#include <iostream>

namespace weave {

using point = vec2<float>;
using rectangle = geo::rectangle<float>;
using triangle = geo::triangle<float>;
using circle = geo::circle<float>;

template <class T>
auto& operator<<(std::ostream& os, const vec<T, 2>& v) {
  os << "{" << v.x << " " << v.y << "}";
  return os;
}

} // weave