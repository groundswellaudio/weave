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
    auto obj_ty = (type) decl_of(b);
    if (!is_compound_assign(op))
      obj_ty = make_const(obj_ty);
    obj_ty = make_lvalue_reference(obj_ty);

    b << ^ [operand_type, op, obj_ty] struct 
    {
      constexpr decltype(auto) %name(op) (this %typename(obj_ty) self, %typename(operand_type) rhs) {
        return self.template apply_op<op>(rhs);
      }
    };
  }
}

template <int N>
consteval auto declare_operators(class_builder& b, type operand_type, const operator_kind(&ops)[N])
{
  for (auto op : ops)
  {
    impl::declare_op(b, operand_type, op);
  }
}

consteval auto declare_arithmetic(class_builder& b, type operand_type) 
{
  auto ref_t = make_lvalue_reference(decl_of(b));
  declare_operators(b, operand_type, math_compound);
  declare_operators(b, operand_type, math_op);
}