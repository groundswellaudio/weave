#pragma once

template <class T, std::meta::field_decl FD>
struct lifted_field_lens
{
  constexpr decltype(auto) operator()(auto&& head) {
    return head.%(FD);
  }
  
  using value_type = %type_of(FD);
  
  value_type& read(void* state) const { return static_cast<T*>(state)->%(FD); }
  
  void write(value_type val, void* state) const { 
    read(state) = val;
  }
};

consteval expr lens(field_expr f) {
  auto ty = remove_reference(type_of(child(f, 0)));
  return ^ [f, ty] (lifted_field_lens< %typename(ty), field(f) >{});
}

#define LENS(EXPR) simple_lens<decltype(EXPR), [] (auto& state) -> auto&& { return EXPR; }>