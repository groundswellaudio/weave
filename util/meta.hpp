#pragma once

using namespace std::meta;

consteval expr stringify(type t) {
  std::meta::ostream os;
  os << t;
  return make_literal_expr(os);
}