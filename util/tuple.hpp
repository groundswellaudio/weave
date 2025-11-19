#pragma once

#include <kumi/kumi.hpp>
//#include <kumi/utils.hpp>

namespace weave {
  using kumi::tuple;
  
  template <class Fn, class... Ts>
  constexpr void tuple_for_each(Fn&& fn, Ts&&... ts) {
    kumi::for_each(fn, (Ts&&)ts... );
  }
  
  template <class Fn, class... Ts>
  constexpr void tuple_for_each_with_index(Fn&& fn, Ts&&... ts) {
    kumi::for_each_index(fn, (Ts&&)ts... );
  }
  
} // weave