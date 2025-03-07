#pragma once

#include <type_traits>

namespace impl 
{
  using namespace std::meta;
  
  consteval void expand_as_fields(class_builder& b, type_list tl)
  {
    int k = 0;
    for (auto t : tl)
      b << ^^ [t, p = k++] struct { [:t:] name[:cat("m", p):]; };
  }
  
  consteval void collect_fields(std::meta::expr_list& el, std::meta::expr e)
  {
    using namespace std::meta;
    auto cd = decl_of(remove_reference(type_of(e)));
    for (auto f : fields(cd))
      push_back( el, ^^ [e, f] ([:e:].[:f:]) );
  }

  consteval std::meta::expr_list expand_fields(std::meta::expr_list el)
  {
    using namespace std::meta;
    expr_list res;
    for (expr e : el)
      collect_fields(res, e);
    return res;
  }

  consteval std::meta::expr get_by_type(std::meta::type T, std::meta::expr E)
  {
    auto cd =  decl_of(remove_reference(type_of(E)));
    for (auto fd : fields(cd))
      if (type_of(fd) == T)
        return ^^ [fd, E] ( [:E:].[:fd:] );
    std::meta::ostream os;
    os << "type " << T << " not contained in " << type_of(cd);
    error(os);
    return ^^(0);
  }
}

template <class... Ts>
struct tuple
{
  constexpr bool operator<=>(const tuple<Ts...>& ts) const = default;
  
  [: impl::expand_as_fields({^^Ts...}) :];
};

template <unsigned Index, class Tpl>
  requires ( std::meta::is_instance_of(std::meta::remove_reference(^^Tpl), ^^tuple) )
constexpr auto&& get(Tpl&& tpl)
{
  return tpl.[: cat("m", Index) :];
}

template <class T, class Tpl>
  requires ( std::meta::is_instance_of(std::meta::remove_reference(^^Tpl), ^^tuple) )
constexpr auto&& get(Tpl&& tpl)
{
  return [: impl::get_by_type( ^^T, ^^(tpl) ) :];
}

namespace impl
{
  using namespace std::meta;
  consteval type tuple_cat_result_type(type_list tl) 
  {
    template_argument_list args;
    for (auto t : tl)
      for (auto a : template_arguments_of(t))
        push_back(args, a);
    return type_of(substitute(^^tuple, args));
  }
}

template <class... Ts>
  requires ( is_instance_of(remove_reference(^^Ts), ^^tuple) && ... ) // expected-note {{because 'is_instance_of(remove_reference(^^int), ^^tuple)' evaluated to false}}, expected-note {{and 'is_instance_of(remove_reference(^^std::nullptr_t), ^^tuple)' evaluated to false}}
constexpr auto tuple_cat(Ts&&... ts) // expected-note {{candidate template ignored: constraints not satisfied [with Ts = <tuple<int, float> &, int, std::nullptr_t>]}}
{
  using ResultType = [: impl::tuple_cat_result_type( {remove_reference(^^Ts)...} ) :];
  return ResultType{ [:...impl::expand_fields(^^(ts...)):]... };
}

template <class Fn, class... Ts>
  requires ( is_instance_of(remove_reference(^^Ts), ^^tuple) && ... )
constexpr decltype(auto) apply(Fn fn, Ts&&... ts)
{
  return fn( [:...impl::expand_fields(^^(ts...)):]... );
}