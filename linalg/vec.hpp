
#pragma once

#include "gen-operators.hpp"

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
  
  T data[N];
  
  %declare_arithmetic( ^const vec& );
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
  
  constexpr bool operator==(const vec<T, 2>& o) const = default;
  
  constexpr auto operator-() const {
    vec<T, 2> res {*this};
    res.x = -res.x;
    res.y = -res.y;
    return res;
  }
  
  %declare_arithmetic( ^const vec& );
  
  T x, y;
};

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
      auto type = instantiate(^vec, template_arguments{bases[k], dim});
      auto name = cat(cat(vecid, dim), tsuffix[k]);
      append_alias(b, name, type);
    }
  }
} ();

}