#pragma once
#pragma once

#include <iostream>
#include <limits>
#include <source_location>
#include <string_view>

namespace weave {

struct ignore {
  template <class T>
  constexpr ignore(T&&){}
  constexpr ignore(){}
};

struct non_copyable {
  constexpr non_copyable() = default;
  non_copyable(const non_copyable&) = delete;
};

template <class T>
struct tag {};

template <class Fn>
struct on_exit {
  constexpr ~on_exit() { fn(); }
  Fn fn;
};

template <class Fn>
on_exit(Fn) -> on_exit<Fn>;

template <class RT, class O, class... Args>
using member_fn_ptr = RT (O::*)(Args...);

template <class T>
concept complete_type = requires { sizeof(T); };

template <class T>
void debug_log(T val) {
  std::cerr << val << std::endl;
}

template <class T>
T infinity() { return std::numeric_limits<T>::infinity(); }

template <class T>
std::string_view stringify() {
  auto n = std::source_location::current().function_name();
  auto&& prefix = "std::string_view weave::stringify() [T = ";
  auto start = n + sizeof(prefix) - 1;
  auto end = n + std::string_view{n}.size() - 1;
  return {start, end};
}

} // weave