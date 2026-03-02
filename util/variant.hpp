#pragma once

#include <swl/variant.hpp>

namespace weave {
  using swl::variant;
  using swl::in_place_index;
  
  template <class... Ts, class S>
    requires ((std::is_same_v<Ts, S> || ...))
  constexpr bool operator==(const variant<Ts...>& v, const S& s) {
    return holds_alternative<S>(v) && get<S>(v) == s;
  }
} // weave
