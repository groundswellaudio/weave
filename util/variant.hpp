#pragma once

using namespace std::meta;

#include <memory>
#include <utility>

namespace impl
{
  consteval void generate_fields(class_builder& b, type_list tl)
  {
    int k = 0;
    for (auto t : tl)
    {
      b << ^ [t, p = k++] struct { %typename(t) %name(cat("m", p)); };
    }
  }
  
  template <unsigned N>
  struct constant {
    static constexpr auto value = N; 
  };
  
  // We could memoize the traversal of the fields and the construction of the switch 
  // by making the unary visit a struct template parametrised on the variant type
  // but we need eager injection for that
  
  template <class Fn>
  consteval void for_each_field(expr data, Fn fn) {
    auto variant_data = static_cast<class_decl>(remove_reference(type_of(data)));
    int k = 0;
    auto fl = fields(variant_data);
    auto it = begin(fl);
    auto endit = end(fl);
    ++it;
    for (; it != endit; ++it)
      fn(*it, k++);
  }
  
  template <class ValueCtor>
  consteval void generate_unary_visit(function_builder& b, expr data, ValueCtor value_ctor)
  {
    auto variant_data = static_cast<class_decl>(remove_reference(type_of(data)));
    int k = 0;
    auto fl = fields(variant_data);
    auto it = begin(fl);
    auto endit = end(fl);
    ++it;
    for (; it != endit; ++it)
    {
      auto f = *it;
      
      b << ^ [k] { case k: };
      value_ctor( b, ^[data, f]((%data).%(f)) );
      
      /* 
      b << ^ [k, value_ctor, f] 
      {
        case k : 
          %value_ctor( (%data).%(f) );
      }; */ 
      ++k; 
      
      // can't use fragments here until fragment transform within template is implemented
      //append_case(b, make_literal_expr(k++));
      //value_ctor(b, make_field_expr(data, f));
    }
  }

  consteval void generate_variadic_visit(function_builder& b, expr fn, expr_list pack, int index, expr_list call_args)
  {
    if (index == size(pack))
    {
      b << ^ [fn, call_args] { return (%fn)( %...call_args... ); };
      return;
    }
    
    /* 
    b << ^ [fn, call_args, pack, index] 
    {
      switch( (%pack[index]).index() )
      {
        for_each_field( pack[index], [] ()
        %generate_unary_visit(fn, pack, index + 1, )
      } 
    }; */ 
    
    expr var = pack[index];
    expr index_expr = ^[var] ((%var).index()); 
    append_switch(b, index_expr, [&] (function_builder& b)
    {
      expr data_expr = ^ [var] ((%var).data);
    
      auto continuation = [&] (function_builder& b, field_expr var_field) 
      {
        auto next_args = call_args;
        push_back(next_args, var_field);
        auto next_index = index + 1;
        generate_variadic_visit(b, fn, pack, next_index, next_args);
      };
    
      generate_unary_visit(b, data_expr, continuation);
    });
  }

  consteval void generate_visit_with_index(function_builder& b, expr data, expr vis)
  {
    for_each_field(data, [&b, data, vis] (decl d, int index) {
      b << ^ [data, d, vis, index] {
        case index : return (%vis)( (%data).%(d), constant<index>{} );
      };
    });
  }
  
  consteval void generate_visit(function_builder& b, expr data, expr vis) {
    for_each_field(data, [&b, data, vis] (field_decl fd, int index) {
      b << ^ [data, vis, index, fd] {
        case index : return (%vis)( (%data).%(fd) );
      };
    });
  }
  
  struct destruct_element 
  {
    template <class T>
    constexpr void operator()(T& elem) { elem.~T(); }
  };
  
  consteval auto generate_overloads(class_builder& b, type_list tl)
  {
    int k = 0;
    for (auto t : tl)
    {
      b << ^ [t, p = k++] struct 
      {
        constant<p> operator() (%typename(t) x);
      };
    }
  }
  
  template <class... Ts>
  struct overload_selector
  {
    % generate_overloads(type_list{^Ts...});
  };
  
  struct empty_t{};
  
  consteval unsigned find_first_pos(const type_list& tl, type t) {
    unsigned k = 0;
    for (auto e : tl){
      if (t == e)
        return k;
      ++k;
    }
    
    std::meta::ostream os;
    os << "variant::index of : type " << t << " not contained";
    error(os);
    return 0;
  }
  
  template <class T>
  constexpr T declval();
}

struct variant_base {};

template <class... Ts>
struct variant;

template <class Fn, class V>
  requires (is_base_of(^variant_base, remove_reference(^V)))
constexpr decltype(auto) visit_with_index(Fn&& fn, V&& v)
{
  switch(v.index())
  {
    %impl::generate_visit_with_index( ^(v.data), ^(fn) );
    default: 
      std::unreachable();
  }
}

template <class Fn, class V>
  requires (is_base_of(^variant_base, remove_reference(^V)))
constexpr decltype(auto) visit(Fn&& fn, V&& v)
{
  switch(v.index()) 
  {
    %impl::generate_visit(^(v.data), ^(fn));
  }
  //%impl::generate_variadic_visit(^(fn), expr_list{^(vs)...}, 0, expr_list{});
  std::unreachable();
} 

/* 
template <class Fn, class... Vs>
  requires (is_base_of(^variant_base, remove_reference(^Vs)) && ...)
constexpr decltype(auto) visit(Fn&& fn, Vs&&... vs)
{
  %impl::generate_variadic_visit(^(fn), expr_list{^(vs)...}, 0, expr_list{});
  std::unreachable();
} */ 

template <unsigned Idx>
struct in_place_index_t {};

template <unsigned Idx>
static constexpr auto in_place_index = in_place_index_t<Idx>{};

template <class... Ts>
struct variant : variant_base
{
  static constexpr bool trivial_dtor = (is_trivially_destructible(^Ts) && ...);
  
  template <class T>
  using ctor_select = decltype(impl::overload_selector<Ts...>{}({impl::declval<T>()}));
  
  static consteval type alternative(unsigned idx) { return type_list{^Ts...}[idx]; }
  static consteval unsigned index_of(type T)      { return impl::find_first_pos(type_list{^Ts...}, T); }
  
  template <int N>
  using alternative_t = %alternative(N);
  
  template <class T>
    requires (!is_instance_of(remove_reference(^T), ^variant) && requires { typename ctor_select<T&&>; })
  constexpr variant(T&& t)
  : data{}
  {
    constexpr auto emplace_idx = ctor_select<T&&>::value;
    data.template emplace<emplace_idx>( (T&&) t );
    index_m = emplace_idx;
  }
  
  template <unsigned Index, class... Args>
  constexpr variant(in_place_index_t<Index>, Args&&... args) 
  requires (Index < sizeof...(Ts))
  : data{}
  {
    data.template emplace<Index>( (Args&&)args... );
    index_m = Index;
  }
  
  constexpr variant(const variant& v)
  : data{}
  {
    emplace_from(v);
  }
  
  constexpr variant(variant&& v)
    noexcept ((is_nothrow_move_constructible(^Ts) && ...))
    requires (is_move_constructible(^Ts) && ...)
  : data{}
  {
    emplace_from((variant&&) v);
  }
  
  template <unsigned N, class... Args>
  constexpr alternative_t<N> emplace(Args&&... args) {
    destroy();
    auto& res = data.template emplace<N>( (Args&&) args... );
    index_m = N;
    return res;
  }
  
  constexpr variant& operator=(const variant& o) 
    requires (is_assignable(^Ts, ^Ts) && ...)
  {
    destroy();
    emplace_from(o);
    return *this;
  }
  
  constexpr auto index() const {
    return index_m;
  }
  
  union Data 
  {
    constexpr Data() 
    : xxx{}
    {}
    
    template <unsigned N, class... Args>
    constexpr auto& emplace(Args&&... args) {
      return *std::construct_at( &this->%(cat("m", N)), (Args&&)args... );
    }
    
    constexpr ~Data() 
      requires (not trivial_dtor)
    {
    }
    
    constexpr ~Data()
      requires (trivial_dtor)
    = default;
    
    impl::empty_t xxx;
    %impl::generate_fields(type_list{^Ts...});
  };
  
  constexpr ~variant() 
    requires (not trivial_dtor)
  {
    visit( impl::destruct_element{}, *this );
  }
  
  constexpr ~variant() 
    requires (trivial_dtor)
  = default;
  
  template <unsigned Index>
  constexpr auto&& get(this auto&& self) { 
    return self.data.%(fields(^Data)[Index + 1]); 
  }
  
  template <class T>
  constexpr auto&& get(this auto&& self) { return self.template get<index_of(^T)>(); }
  
  Data data;
  
  private : 
  
  template <unsigned Idx, class... Args>
  constexpr void emplace_no_reset(Args&&... args) {
    data.template emplace<Idx>( (Args&&)args... );
  }
  
  template <class V>
  constexpr void emplace_from(V&& o) 
  {
    visit_with_index( [this] (auto& elem, auto idx) {
      index_m = idx.value;
      this->emplace_no_reset<idx.value>( elem );
    }, (V&&) o);
  }
  
  constexpr void destroy() {
    if constexpr ( not trivial_dtor )
      visit( impl::destruct_element{}, *this );
  }
  
  unsigned char index_m;
};

template <unsigned Index, class V>
  requires (is_derived_from(remove_reference(^V), ^variant_base))
constexpr auto&& get(V&& var) {
  return var.data.%(cat("m", Index));
}

template <class T, class V>
  requires (is_derived_from(remove_reference(^V), ^variant_base))
constexpr auto&& get(V&& var) {
  return var.template get<T>();
}

template <class T, class... Vs>
constexpr bool holds_alternative(const variant<Vs...>& v) {
  constexpr auto idx = impl::find_first_pos({^Vs...}, ^T);
  //static_assert( idx != -1, "type not contained in variant" );
  return idx == v.index();
} 