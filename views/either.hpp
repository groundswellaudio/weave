#pragma once

#include "views_core.hpp"

#include "../util/variant.hpp"
#include "../util/optional.hpp"

namespace weave::views {

template <class View>
struct maybe : view_sequence_base {
  
  template <class F>
  maybe(F f, View v) : flag{static_cast<bool>(f)}, view{v} {}
  
  maybe(bool flag, View v) : flag{flag}, view{v} {}
  
  using Self = maybe<View>;
  
  template <class S>
  void seq_build(auto consumer, auto&& b, S& state)
  {
    if (!flag) 
      return;
    view.seq_build(consumer, b, state);
  }
  
  template <class State>
  rebuild_result seq_rebuild(Self& Old, auto&& seq_updater, const build_context& ctx, State& s) 
  {
    if (flag == Old.flag)
    {
      if (Old.flag)
        view.seq_rebuild(Old.view, seq_updater, ctx, s);
    }
    else if (!Old.flag)
    {
      view.seq_build(seq_updater.consume_fn(), ctx, s);
      return {};
    }
    else
    {
      Old.view.seq_destroy(seq_updater.destroy_fn(), ctx.context());
    }
    return {};
  }
  
  bool flag; 
  View view;
};

template <class V>
maybe(bool, V) -> maybe<V>;

namespace impl {
  
  template <unsigned Idx>
  struct Index {};
  
  template <unsigned Max, unsigned Offset = 0>
  constexpr auto with_index(unsigned index, auto fn) -> decltype(fn(Index<0>{})) {
    switch(index) {
      case 0 : return fn(Index<0 + Offset>{});
      case 1 : return fn(Index<1 + Offset>{});
      case 2 : return fn(Index<2 + Offset>{});
      case 3 : return fn(Index<3 + Offset>{});
      case 4 : return fn(Index<4 + Offset>{});
      case 5 : return fn(Index<5 + Offset>{});
      case 6 : return fn(Index<6 + Offset>{});
      case 7 : return fn(Index<7 + Offset>{});
      default : 
        return with_index<Max, Offset - 8>(index - 8, fn);
    }
  }
  
  template <class... Ts, class... Vs>
  variant<Ts...> make_variant(unsigned index, Vs&&... vs) {
    return with_index<sizeof...(Vs)>(index, [&vs...] <unsigned I> (Index<I>) {
      return variant<Ts...>{ swl::in_place_index<I>, get<I>(forward_as_tuple(vs...)) };
    });
  }
  
  template <class... Ts>
  struct overloaded : Ts... {
    using Ts::operator()...;
  };
  
  template <class... Ts>
  overloaded(Ts...) -> overloaded<Ts...>;

  template <class... Ts, class... Fns>
  variant<Ts...> map_variant(auto&& var, Fns... fns) {
    return visit( [&fns...] (auto&& v) { return variant<Ts...>{overloaded{fns...}(v)}; }, var );
  }
}

template <class... Ts>
struct either : view_sequence_base {
  
  template <class... Vs>
  either(variant<Vs...> v, auto... fn) : body{impl::map_variant<Ts...>(v, fn...)} 
  {
  }
  
  either(unsigned index, Ts... vs) : body{impl::make_variant<Ts...>(index, (Ts&&)vs...)} 
  {
  }
  
  void seq_build(auto consumer, auto&& ctx, auto& state) {
    visit( [&] (auto& elem) {
      elem.seq_build(consumer, ctx, state);
    }, body);
  }
  
  rebuild_result seq_rebuild(auto& Old, auto&& updater, build_context ctx, auto& state) {
    if (Old.body.index() == body.index()) {
      return visit_with_index( [&] (auto& elem, auto index) {
        return elem.seq_rebuild(get<index.value>(Old.body), updater, ctx, state);
      }, body);
    }
    else {
      visit( [&] (auto& elem) {
        elem.seq_destroy(updater.destroy_fn(), ctx.context());
      }, Old.body);
      visit( [&] (auto& elem) {
        elem.seq_build(updater.consume_fn(), ctx, state);
      }, body);
      return rebuild_result{}; 
    }
  }
  
  void seq_destroy(auto Destroy, application_context& ctx) {
    visit( [&] (auto& elem) {
      elem.seq_destroy(Destroy, ctx);
    }, body);
  }
  
  variant<Ts...> body;
};

template <class... Ts>
either(int index, Ts... ts) -> either<Ts...>;

template <class... Vs, class... Fn>
either(variant<Vs...>, Fn... fns) -> either<std::invoke_result_t<Fn, Vs>...>;

} // views