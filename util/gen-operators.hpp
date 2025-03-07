#pragma once

#include <span>

using namespace std::meta;

static constexpr operator_kind math_op[] = {
  operator_kind::add,
  operator_kind::sub,
  operator_kind::mul,
  operator_kind::div
};

static constexpr operator_kind math_compound[] = {
  operator_kind::add_assign,
  operator_kind::sub_assign,
  operator_kind::mul_assign,
  operator_kind::div_assign
};

namespace impl {
  consteval void declare_op(class_builder& b, type operand_type, operator_kind op)
  {
    auto self_ty = type_of(b); 
    
    if (!is_compound_assign(op))
      self_ty = add_const(self_ty);
    
    b << ^^ [self_ty, operand_type, op] struct 
    {
      constexpr decltype(auto) name[:op:] (this [:self_ty:]& self, [:operand_type:] rhs) {
        return self.template apply_op<op>(rhs);
      }
    };
  }
}

consteval auto declare_operators(class_builder& b, type operand_type, std::span<const operator_kind> ops)
{
  for (auto op : ops)
    impl::declare_op(b, operand_type, op);
}

consteval auto declare_arithmetic(class_builder& b, type operand_type) 
{
  declare_operators(b, operand_type, math_compound);
  declare_operators(b, operand_type, math_op);
}