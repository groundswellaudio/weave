#pragma once

#include <functional>
#include "util/util.hpp"

namespace weave {

struct empty_lens {};

template <class Fn>
struct invocable_wrapper {
  
  template <class... Args>
  constexpr decltype(auto) operator() (this auto&& self, Args&&... args) 
  {
    return ( std::invoke(self.fn, (Args&&)args...) );
  }
  
  Fn fn;
};

namespace detail {

template <class T>
struct is_atomic { static constexpr bool value = false; };

template <class T>
struct is_atomic<std::atomic<T>> { 
  static constexpr bool value = true; 
  using underlying = T; 
};

template <class V>
auto&& unwrap_atomic(const V& v) { return v; }

template <class T>
auto unwrap_atomic(const std::atomic<T>& av) {
  return av.load(std::memory_order_relaxed);
}

template <class T>
static constexpr bool false_ = sizeof(T) > 1e10; 

}

template <class S>
decltype(auto) apply_read(S& state, auto&& fn) {
  if constexpr( requires { state.apply_read(fn); } ) {
    auto res = detail::unwrap_atomic(state.apply_read(fn));
    static_assert( !detail::is_atomic<decltype(res)>::value );
    return res;
  }
  else { 
    return detail::unwrap_atomic(fn(state));
  }
}

template <class S>
void apply_write(S& state, auto&& fn) {
  if constexpr ( requires {state.apply_write(fn);} ) 
    state.apply_write(fn);
  else
    fn(state);
}

template <class A, class B>
struct lens_readwrite {
  
  decltype(auto) read(auto& state) {
    return (apply_read(state, r));
  }
  
  void write(auto& state, auto&& val) const {
    apply_write(state, [this, &val] (auto& s) {
      w(s, (decltype(val)&&) val);
    });
  }
  
  A r;
  B w;
};

template <class Fn>
struct invocable_lens {

  decltype(auto) read(auto& s) const {
    return apply_read(s, fn);
  }
  
  void write(auto& s, auto&& val) const {
    auto f = [this, &val] (auto& s) { fn(s) = (decltype(val)&&)val; };
    return apply_write(s, f);
  }
  
  Fn fn;
};

template <class A, class B>
auto make_lens(lens_readwrite<A, B> l) {
  return l;
}

template <class L>
auto make_lens(L l) {
  return invocable_lens{invocable_wrapper{l}};
}

template <class A, class B>
auto readwrite(A val, B b) {
  return lens_readwrite{ [val] (ignore) { return val; }, invocable_wrapper<B>{b} };
}

template <class T>
using make_lens_result = decltype(make_lens(std::declval<T>()));

}