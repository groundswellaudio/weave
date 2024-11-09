using namespace std::meta;

template <class... Ts>
struct tuple; 

namespace impl 
{

  consteval void generate_fields(class_builder& b, type_list tl)
  {
    int k = 0;
    for (auto t : tl)
      append_field(b, cat("m", k++), t);
  }


  consteval void generate_get(method_builder& b, int n)
  {
    append_return( b, make_field_expr(b, fields(parent(decl_of(b)))[n]) );
  }

  consteval void append_field_list(expr_list& res, expr self)
  {
    auto cd = (class_decl) remove_reference(type_of(self));
    for (auto f : fields(cd))
      push_back(res, make_field_expr(self, f));
  }

  consteval expr_list get_field_list(expr self)
  {
    expr_list res;
    append_field_list(res, self);
    return res;
  }

  consteval expr generate_eq_comp(expr a, expr b) 
  {
    expr res = ^(true);
    auto cd = (class_decl) remove_reference(type_of(a));

    for (auto f : fields(cd)) 
    {
      auto comp = make_operator_expr(operator_kind::eq,
          make_field_expr(a, f),
          make_field_expr(b, f));
    
      res = make_operator_expr(operator_kind::land, res, comp);
    }
  
    return res;
  }
  
  consteval expr generate_tuple_cat(type_list tl, expr_list el)
  {
    template_arguments args;
  
    for (auto t : tl)
      for (auto a : arguments((class_template_type)(t)))
        push_back(args, a); 
    auto rt = instantiate(^tuple, args);
  
    expr_list inits;
    for (auto e : el)
      append_field_list(inits, e);
  
    return make_construct_expr( rt, inits );
  }

  consteval expr generate_apply(expr fn, expr_list tpls)
  {
    expr_list args;
    for (auto elem : tpls)
      append_field_list(args, elem);
    return make_call_expr(fn, args);
  }
  
  consteval void generate_for_each(function_builder& b, expr tpl, expr fn) 
  {
    auto cd = (class_decl) remove_reference(type_of(tpl));
    for (auto f : fields(cd))
      append_expr(b, make_call_expr(fn, expr_list{make_field_expr(tpl, f)}));
  }
  
}

template <class... Ts>
struct tuple
{
  template <class Fn>
  constexpr decltype(auto) apply(Fn fn) {
    static constexpr auto args = impl::get_field_list(^(*this));
    return %make_call_expr(^(fn), args);
  }
  
  template <int N>
  constexpr auto&& get() {
    %impl::generate_get(N);
  }
  
  template <int N>
  constexpr auto&& get() const {
    return const_cast<tuple*>(this)->get<N>();
  }
  
  constexpr bool operator==(const tuple<Ts...>& ts) const {
    return %impl::generate_eq_comp(^(*this), ^(ts));
  }
  
  %impl::generate_fields(type_list{^Ts...});
};

template <class... Ts>
  requires ( is_instance_of(remove_reference(^Ts), ^tuple) && ... ) 
constexpr auto tuple_cat(Ts&&... ts)
{
  return %impl::generate_tuple_cat( type_list{remove_reference(^Ts)...}, expr_list{^(ts)...} );
}

template <class Fn, class... Ts>
  requires ( is_instance_of(remove_reference(^Ts), ^tuple) && ... )
constexpr decltype(auto) apply(Fn fn, Ts&&... ts)
{
  return %impl::generate_apply( ^(fn), ^({ts...}) );
}

template <class Tpl, class Fn>
  requires ( is_instance_of(remove_reference(^Tpl), ^tuple) )
constexpr void tuple_for_each(Tpl&& tpl, Fn fn)
{
  %impl::generate_for_each(^(tpl), ^(fn));
}

template <unsigned Index, class Tpl>
  requires ( is_instance_of(remove_reference(^Tpl), ^tuple) )
constexpr auto&& get(Tpl&& tpl) {
  return tpl.template get<Index>();
}

namespace impl {
  consteval void gen_visit(function_builder& b, expr self, expr index, expr fn)
  {
    auto sb = [fn, self] (auto& b) {
      auto cd = (class_decl) remove_reference(type_of(self));
      int k = 0;
      for (auto f : fields(cd))
      {
        append_case( b, make_literal_expr(k++), [f, fn, self] (auto& c) {
          auto args = expr_list{make_field_expr(self, f)};
          append_return(c, make_call_expr(fn, args));
        });
      }
    };
    append_switch(b, index, sb);
  }
}

template <class Tpl, class Fn>
  requires ( is_instance_of(remove_reference(^Tpl), ^tuple) )
constexpr decltype(auto) visit_at_index(Tpl&& tpl, int index, Fn&& fn)
{
  %impl::gen_visit(^(tpl), ^(index), ^(fn)); 
}