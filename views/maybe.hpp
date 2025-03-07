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
  rebuild_result seq_rebuild(Self& Old, auto&& seq_updater, const widget_updater& up, State& s) 
  {
    if (flag == Old.flag)
    {
      if (Old.flag)
        view.seq_rebuild(Old.view, seq_updater, up, s);
    }
    else if (!Old.flag)
    {
      auto nb = up.builder();
      view.seq_build(seq_updater.consume_fn(), nb, s);
      return {};
    }
    else
    {
      Old.view.seq_destroy(seq_updater.destroy_fn());
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
  
  consteval void with_index(function_builder& b, unsigned max, expr fn) {
    for (int k = 0; k < max; ++k)
      b << ^^ [k, fn] { case k : return [:fn:](Index<k>{}); };
  }
  
  template <unsigned Max>
  constexpr auto with_index(unsigned index, auto fn) -> decltype(fn(Index<0>{})) {
    switch(index) {
      [:with_index(Max, ^^(fn)):];
      default : 
        std::unreachable();
    }
  }
  
  template <class... Ts, class... Vs>
  variant<Ts...> make_variant(unsigned index, Vs&&... vs) {
    return with_index<sizeof...(Vs)>(index, [&vs...] <unsigned I> (Index<I>) {
      return variant<Ts...>{ in_place_index<I>, [:(^^(vs...)[I]):] };
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
  
  void seq_build(auto consumer, auto&& b, auto& state) {
    visit( [&] (auto& elem) {
      elem.seq_build(consumer, b, state);
    }, body);
  }
  
  rebuild_result seq_rebuild(auto& Old, auto&& updater, auto& up, auto& state) {
    if (Old.body.index() == body.index()) {
      return visit_with_index( [&] (auto& elem, auto index) {
        return elem.seq_rebuild(get<index.value>(Old.body), updater, up, state);
      }, body);
    }
    else {
      visit( [&] (auto& elem) {
        elem.seq_destroy(updater.destroy_fn());
      }, Old.body);
      visit( [&] (auto& elem) {
        elem.seq_build(updater.consume_fn(), up.builder(), state);
      }, body);
      return rebuild_result{}; 
    }
  }
  
  void seq_destroy(auto Destroy) {
    visit( [&] (auto& elem) {
      elem.seq_destroy(Destroy);
    }, body);
  }
  
  variant<Ts...> body;
};

template <class... Ts>
either(int index, Ts... ts) -> either<Ts...>;

template <class... Vs, class... Fn>
either(variant<Vs...>, Fn... fns) -> either<std::invoke_result_t<Fn, Vs>...>;

} // views