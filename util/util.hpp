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