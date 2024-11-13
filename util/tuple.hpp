#pragma once

using namespace std::meta;

namespace impl 
{
  consteval void expand_as_fields(class_builder& b, type_list tl)
  {
    int k = 0;
    for (auto t : tl)
      b << ^ [t, p = k++] struct { %typename(t) %name(cat("m", p)); };
  }
  
  consteval void collect_fields(std::meta::expr_list& el, std::meta::expr e)
  {
    using namespace std::meta;
    class_decl cd = static_cast<class_decl>( remove_reference(type_of(e)) );
    for (field_decl f : fields(cd))
      push_back( el, ^ [e, f] ((%e).%(f)) );
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
    auto cd = (std::meta::class_decl) remove_reference(type_of(E));
    for (field_decl fd : fields(cd))
      if (type_of(fd) == T)
        return ^ [fd, E] ( (%E).%(fd) );
    std::meta::ostream os;
    os << "type " << T << " not contained in " << (type) cd;
    error(os);
    return ^(0);
  }
}

template <auto N>
struct constant {
  static constexpr auto value = N;
  constexpr operator decltype(N) () const { return N; } 
};

template <class... Ts>
struct tuple
{
  constexpr bool operator<=>(const tuple<Ts...>& ts) const = default;
  
  %impl::expand_as_fields(type_list{^Ts...});
};

template <unsigned Index, class Tpl>
  requires ( std::meta::is_instance_of(std::meta::remove_reference(^Tpl), ^tuple) )
constexpr auto&& get(Tpl&& tpl)
{
  constexpr class_decl cd = static_cast<std::meta::class_decl>(remove_reference(^Tpl));
  return tpl.%(fields(cd)[Index]);
}

template <class T, class Tpl>
  requires ( std::meta::is_instance_of(std::meta::remove_reference(^Tpl), ^tuple) )
constexpr auto&& get(Tpl&& tpl)
{
  return %impl::get_by_type( ^T, ^(tpl) );
}

namespace impl
{
  consteval type tuple_cat_result_type(type_list tl) 
  {
    template_arguments args;
    for (auto t : tl)
      for (auto a : arguments((class_template_type)(t)))
        push_back(args, a); 
    return instantiate(^tuple, args);
  }
  
  consteval void gen_tuple_for_each(function_builder& b, expr fn, expr tpl)
  {
    for (auto e : expand_fields(expr_list{tpl}))
      b << ^ [fn, e] ( (%fn)(%e) );
  }
  
  consteval void gen_tuple_for_each_with_index(function_builder& b, expr fn, expr tpl) {
    int k = 0;
    for (auto e : expand_fields(expr_list{tpl}))
      b << ^ [fn, e, p = k++] ( (%fn)(%e, constant<p>{}) );
  }
}

template <class... Ts>
  requires ( is_instance_of(remove_reference(^Ts), ^tuple) && ... ) 
constexpr auto tuple_cat(Ts&&... ts) 
{
  using ResultType = %impl::tuple_cat_result_type( {remove_reference(^Ts)...} );
  return ResultType{ %...impl::expand_fields(^(ts...))... };
}

template <class Fn, class... Ts>
  requires ( is_instance_of(remove_reference(^Ts), ^tuple) && ... )
constexpr decltype(auto) apply(Fn fn, Ts&&... ts)
{
  return fn( %...impl::expand_fields(^(ts...))... );
}

template <class Fn, class Tpl>
  requires (is_instance_of(remove_reference(^Tpl), ^tuple))
constexpr void tuple_for_each(Tpl&& tpl, Fn fn) {
  %impl::gen_tuple_for_each(^(fn), ^(tpl));
}

template <class Fn, class Tpl>
  requires (is_instance_of(remove_reference(^Tpl), ^tuple))
constexpr void tuple_for_each_with_index(Tpl&& tpl, Fn fn) {
  %impl::gen_tuple_for_each_with_index(^(fn), ^(tpl));
}