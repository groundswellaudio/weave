
#include "gen-operators.hpp"

using namespace std::meta;

static constexpr operator_kind add_sub_c[] = {
  operator_kind::add_assign, operator_kind::sub_assign
};

static constexpr operator_kind add_sub[] {
  operator_kind::add, operator_kind::sub
};

template <class T, unsigned N, unsigned M = N>
struct matrix
{
  using Self = matrix<T, N, M>;
  
  template <operator_kind Op>
    requires (std::meta::is_compound_assign(Op))
  constexpr auto& apply_op(const Self& o)
  {
    for (int k = 0; k < N; ++k)
      (%make_operator_expr(Op, ^(data[k]), ^(o.data[k])));
    return *this;
  }
  
  template <operator_kind Op, class Arg>
    requires (not std::meta::is_compound_assign(Op))
  constexpr auto apply_op(Arg&& o) const 
  {
    auto Res {*this};
    (% make_operator_expr(std::meta::compound_equivalent(Op), ^(Res), ^(o)) );
    return Res;
  }
  
  template <operator_kind Op>
    requires (std::meta::is_compound_assign(Op))
  constexpr auto& apply_op(T val) 
  {
    for (int k = 0; k < N; ++k)
      (%make_operator_expr(Op, ^(data[k]), ^(val)));
    return *this;
  }
  
  constexpr bool operator==(const Self& o) const {
    for (int k = 0; k < N; ++k)
      if (data[k] != o.data[k])
        return false;
    return true;
  }
  
  constexpr bool operator!=(const Self& o) const {
    return not (*this == o);
  }
  
  T data[N * M];
  
  %declare_operators(^const Self&, add_sub_c);
  %declare_operators(^const Self&, add_sub);
  %declare_arithmetic(^T);
};