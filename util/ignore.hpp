#pragma once

struct ignore {
  template <class T>
  constexpr ignore(T&&){}
  constexpr ignore(){}
};