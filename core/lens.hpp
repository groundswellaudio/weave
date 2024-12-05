#pragma once

struct empty_lens {};

template <class... Ts>
struct composed_lens {
  decltype(auto) operator()(auto&& v) const { return v; }
};

consteval std::meta::type last_input_type(std::meta::type_list tl) {
  auto last = tl[size(tl) - 1];
  auto cd = (class_decl) last;
  return (type) (alias_type_decl) lookup(cd, "input");
}

template <class First, class... Tail>
struct composed_lens<First, Tail...> {
  
  using input = %last_input_type(type_list{^First, ^Tail...});
  
  decltype(auto) operator()(auto& v) const { return first(tail(v)); }
  
  First first;
  composed_lens<Tail...> tail;
};

template <class T, std::meta::field_decl FD>
struct lifted_field_lens
{
  using input = T;
  using value_type = %type_of(FD);
  
  constexpr decltype(auto) operator()(auto&& head) const { return (head.%(FD)); }
  
  decltype(auto) read(const T& state) { return (state.%(FD)); }
  void write(T& state, value_type val) const { (*this)(state) = val; }
};

consteval expr lens(field_expr f) {
  auto ty = remove_reference(type_of(child(f, 0)));
  return ^ [f, ty] (lifted_field_lens< %typename(ty), field(f) >{});
}

#define LENS(EXPR) simple_lens<decltype(EXPR), [] (auto& state) -> auto&& { return EXPR; }>

template <class Range>
struct range_element_lens 
{
  using input = Range;
  decltype(auto) operator()(Range& range) const { return (range[index]); }
  int index;
};

template <class LensA, class LensB>
constexpr auto compose_lens(LensA A, LensB B) {
  if constexpr ( ^LensA == ^empty_lens )
    return B;
  else if constexpr ( ^LensB == ^empty_lens )
    return A;
  else 
    return composed_lens<LensB, LensA>{B, {A}};
}

template <class A, class B>
struct lens_readwrite {
  
  decltype(auto) operator()(auto& state) {
    auto _ = state.read_scope();
    return (std::invoke(r, state));
  }
  
  void operator()(auto& state, auto&& val) {
    auto _ = state.write_scope();
    std::invoke(w, state, val);
  }
  
  decltype(auto) read(auto& state) {
    auto _ = state.read_scope();
    return (std::invoke(r, state));
  }
  
  void write(auto& state, auto&& val) {
    auto _ = state.write_scope();
    std::invoke(w, state, val);
  }
  
  A r;
  B w;
};

template <class Fn>
struct simple_lens {

  decltype(auto) read(auto& s) {
    if constexpr (remove_reference(type_of(^s)) == ^void*) 
      return (fn(s));
    else 
      return (s.apply_read([this] (auto& s) -> decltype(auto) { return (std::invoke(fn, s)); }));
  }
  
  void write(auto& s, auto&& val) {
    if constexpr (remove_reference(type_of(^s)) == ^void*)
      std::invoke(fn, s) = (decltype(val)&&)val;
    else
      s.apply_write( [this, &val] (auto& s) {
        std::invoke(fn, s) = (decltype(val)&&)val;
      });
  }
  
  Fn fn;
};

template <class A, class B>
auto readwrite(A a, B b) {
  return lens_readwrite{a, b};
}

template <class L>
auto make_lens(L l) {
  if constexpr (std::is_member_pointer_v<L>)
    return simple_lens{l};
  else if constexpr (is_instance_of(^L, ^lens_readwrite))
    return l;
  else 
    return simple_lens{l};
}

template <class T>
using make_lens_result = decltype(make_lens(std::declval<T>()));

/* 
template <class... Ls, class LensB>
auto compose_lens(composed_lens<Ls...> A, LensB B) {
  if constexpr 
  return composed_lens{B, A};
} */ 
