
#pragma once

#include "gen-operators.hpp"
#include <algorithm>
#include <cmath>

using namespace std::meta;

template <class T, unsigned N>
struct vec 
{
  template <operator_kind Op>
    requires (std::meta::is_compound_assign(Op))
  constexpr auto& apply_op(const vec<T, N>& o)
  {
    for (int k = 0; k < N; ++k)
      (%make_operator_expr(Op, ^(data[k]), ^(o.data[k])));
    return *this;
  }
  
  template <operator_kind Op>
    requires (not std::meta::is_compound_assign(Op))
  constexpr auto apply_op(const vec<T, N>& o) const 
  {
    auto Res {*this};
    (% make_operator_expr(std::meta::compound_equivalent(Op), ^(Res), ^(o)) );
    return Res;
  }
  
  template <operator_kind Op>
    requires (std::meta::is_compound_assign(Op))
  constexpr auto& apply_op(T v) {
    for (int k = 0; k < N; ++k)
      (%make_operator_expr(Op, ^(data[k]), ^(v)));
    return *this;
  }
  
  template <operator_kind Op>
    requires (!std::meta::is_compound_assign(Op))
  constexpr auto apply_op(T v) {
    auto Res {*this};
    (% make_operator_expr(std::meta::compound_equivalent(Op), ^(Res), ^(v)));
    return Res;
  }
  
  constexpr auto operator-() const {
    vec<T, N> res {*this};
    for (auto& e : res.data)
      e = -e;
    return res;
  }
  
  constexpr bool operator==(const vec<T, N>& o) const {
    for (int k = 0; k < N; ++k)
      if (data[k] != o.data[k])
        return false;
    return true;
  }
  
  constexpr bool operator!=(const vec<T, N>& o) const {
    return not (*this == o);
  }
  
  constexpr auto& operator[](this auto&& self, unsigned index) {
    return self.data[index];
  }
  
  T data[N];
  
  %declare_arithmetic( ^const vec& );
  %declare_arithmetic( ^T );
};

template <class T>
struct vec<T, 2>
{
  template <operator_kind Op>
    requires (std::meta::is_compound_assign(Op))
  constexpr auto& apply_op(const vec<T, 2>& o)
  {
    (%make_operator_expr(Op, ^(x), ^(o.x)));
    (%make_operator_expr(Op, ^(y), ^(o.y)));
    return *this;
  }
  
  template <operator_kind Op>
    requires (not std::meta::is_compound_assign(Op))
  constexpr auto apply_op(const vec<T, 2>& o) const 
  {
    auto Res {*this};
    (% make_operator_expr(std::meta::compound_equivalent(Op), ^(Res), ^(o)) );
    return Res;
  }
  
  template <operator_kind Op>
    requires (std::meta::is_compound_assign(Op))
  constexpr auto& apply_op(T v) {
    (%make_operator_expr(Op, ^(x), ^(v)));
    (%make_operator_expr(Op, ^(y), ^(v)));
    return *this;
  }
  
  template <operator_kind Op>
    requires (!std::meta::is_compound_assign(Op))
  constexpr auto apply_op(T v) const {
    auto Res {*this};
    (% make_operator_expr(std::meta::compound_equivalent(Op), ^(Res), ^(v)) );
    return Res;
  }
  
  constexpr bool operator==(const vec<T, 2>& o) const = default;
  
  constexpr auto operator-() const {
    vec<T, 2> res {*this};
    res.x = -res.x;
    res.y = -res.y;
    return res;
  }
  
  constexpr auto& operator[](this auto&& self, unsigned index) {
    return index == 0 ? self.x : self.y;
  }
  
  template <class V>
  constexpr operator vec<V, 2>() const {
    return {static_cast<V>(x), static_cast<V>(y)};
  }
  
  %declare_arithmetic( ^const vec& );
  %declare_arithmetic( ^T );
  
  T x, y;
};

// Element-wise maximum
template <class T>
constexpr vec<T, 2> max(vec<T, 2> a, vec<T, 2> b) {
  return { std::max(a.x, b.x), std::max(a.y, b.y) };
}

template <class T>
constexpr vec<T, 2> min(vec<T, 2> a, vec<T, 2> b) {
  return { std::min(a.x, b.x), std::min(a.y, b.y) };
}

template <class T>
constexpr vec<T, 2> abs(vec<T, 2> o) {
  return { std::abs(o.x), std::abs(o.y) };
}

template <class T>
constexpr T distance(vec<T, 2> a, vec<T, 2> b) {
  auto x = b.x - a.x;
  auto y = b.y - a.y;
  return std::sqrt( x * x + y * y );
}

template <class T>
using vec2 = vec<T, 2>;

namespace gl_types
{

// generate glm like aliases
% [] (namespace_builder& b) 
{
  const char* tsuffix[] { "u", "i", "f", "d", "b" }; 
  type_list bases { ^unsigned, ^int, ^float, ^double, ^bool };
  auto vecid = identifier{"vec"};
  for ( int k = 0; k < size(bases); ++k)
  {
    for (int dim = 2; dim < 5; ++dim)
    {
      auto id = cat(cat(vecid, dim), tsuffix[k]);
      b << ^ [id, dim, b = bases[k]] namespace {
        using %name(id) = vec< %typename(b), dim >;
      };
    }
  }
} ();

}