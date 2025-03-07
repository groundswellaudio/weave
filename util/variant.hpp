using namespace std::meta;

#include <memory>
#include <utility>

namespace weave {

namespace impl
{
  consteval void generate_fields(class_builder& b, type_list tl)
  {
    int k = 0;
    for (auto t : tl)
    {
      b << ^^ [t, p = k++] struct { [:t:] name[:cat("m", p):]; };
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
    auto variant_data = decl_of(remove_reference(type_of(data)));
    int k = 0;
    auto fl = fields(variant_data);
    auto it = begin(fl);
    auto endit = end(fl);
    ++it;
    for (; it != endit; ++it)
      fn(*it, k++);
  }
  
  consteval void generate_variadic_visit(function_builder& b, expr fn, expr_list pack, int index, expr_list call_args)
  {
    if (index == size(pack))
    {
      b << ^^ [fn, call_args] { return [:fn:]( [:...call_args:]... ); };
      return;
    }
    
    auto gen_cases = [&] (function_builder& b) {
      auto data = ^^[pack, index] ( [:pack[index]:].data );
      for_each_field( data, [&] (decl d, int elem_index) {
        auto next_args = call_args;
        push_back(next_args, ^^[pack, index, d] ( [:pack[index]:].data.[:d:] ));
        b << ^^[pack, elem_index, index, fn, next_args] {
          case elem_index : 
            [: generate_variadic_visit(fn, pack, index + 1, next_args) :];
        };
      }); 
    };
    
    b << ^^ [gen_cases, pack, index] 
    {
      switch ( [:pack[index]:].index() )
      {
        [: gen_cases() :];
        default : 
          std::unreachable();
      }
    };
  }

  consteval void generate_visit_with_index(function_builder& b, expr data, expr vis)
  {
    for_each_field(data, [&b, data, vis] (decl d, int index) {
      b << ^^ [data, d, vis, index] {
        case index : return [:vis:]( [:data:].[:d:], constant<index>{} );
      };
    });
  }
  
  consteval void generate_visit(function_builder& b, expr data, expr vis) {
    for_each_field(data, [&b, data, vis] (decl fd, int index) {
      b << ^^ [data, vis, index, fd] {
        case index : return [:vis:]( [:data:].[:fd:] );
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
      b << ^^ [t, p = k++] struct 
      {
        constant<p> operator() ([:t:] x);
      };
    }
  }
  
  template <class... Ts>
  struct overload_selector
  {
    [: generate_overloads(type_list{^^Ts...}) :];
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
}

struct variant_base {};

template <class... Ts>
struct variant;

template <class Fn, class V>
  requires (is_base_of(^^variant_base, remove_reference(^^V)))
constexpr decltype(auto) visit_with_index(Fn&& fn, V&& v)
{
  switch(v.index())
  {
    [: impl::generate_visit_with_index( ^^(v.data), ^^(fn) ) :];
  }
}

template <class Fn, class... Vs>
  requires (is_base_of(^^variant_base, remove_reference(^^Vs)) && ...)
constexpr decltype(auto) visit(Fn&& fn, Vs&&... vs)
{
  [: impl::generate_variadic_visit(^^(fn), ^^(vs...), 0, expr_list{}) :];
}

template <unsigned Idx>
struct in_place_index_t {};

template <unsigned Idx>
static constexpr auto in_place_index = in_place_index_t<Idx>{};

template <class... Ts>
struct variant : variant_base
{
  static constexpr bool trivial_dtor = (is_trivially_destructible(^^Ts) && ...);
  using ctor_selector = impl::overload_selector<Ts...>;
  
  static consteval type alternative(unsigned idx) { return type_list{^^Ts...}[idx]; }
  static consteval unsigned index_of(type T)      { return impl::find_first_pos(type_list{^^Ts...}, T); }
  
  template <class... Args>
  constexpr variant(Args&&... args)
  requires ((sizeof...(Args) > 0) && requires { ctor_selector{}({(Args&&)args...}); })  // note the brace around args here
  : data{}
  {
    constexpr auto emplace_idx = decltype( ctor_selector{}({(Args&&)args...}) )::value;
    data.template emplace<emplace_idx>( (Args&&) args... );
    index_m = emplace_idx;
  }
  
  template <unsigned Index, class... Args>
  constexpr variant(std::in_place_index_t<Index>, Args&&... args) 
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
    noexcept ((is_nothrow_move_constructible(^^Ts) && ...))
    requires (is_move_constructible(^^Ts) && ...)
  : data{}
  {
    emplace_from((variant&&) v);
  }
  
  template <unsigned N, class... Args>
  constexpr void emplace(Args&&... args) {
    destroy();
    data.template emplace<N>( (Args&&) args... );
    index_m = N;
  }
  
  constexpr variant& operator=(const variant& o) 
    requires (is_assignable(^^Ts) && ...)
  {
    destroy();
    emplace_from(o);
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
    constexpr void emplace(Args&&... args) {
      std::construct_at( &this->[: cat("m", N) :], (Args&&)args... );
    }
    
    constexpr ~Data() 
      requires (not trivial_dtor)
    {
    }
    
    constexpr ~Data()
      requires (trivial_dtor)
    = default;
    
    impl::empty_t xxx;
    [: impl::generate_fields(type_list{^^Ts...}) :];
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
    return self.data.[: cat("m", Index) :];
  }
  
  template <class T>
  constexpr auto&& get(this auto&& self) { return self.template get<index_of(^^T)>(); }
  
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

} // weave