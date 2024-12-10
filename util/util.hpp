#pragma once

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