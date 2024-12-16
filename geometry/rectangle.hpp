#pragma once

#include <util/vec.hpp>

namespace geo {

template <class T>
struct rectangle {
  
  bool contains(vec2<T> p) const {
    return p.x >= origin.x && p.x <= origin.x + size.x 
           && p.y >= origin.y && p.y <= origin.y + size.y;  
  }
  
  vec2<T> origin, size;
};

}