#pragma once

#include "tuple.hpp"

struct ignore {
  template <class T>
  constexpr ignore(T&&){}
};
