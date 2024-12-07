#pragma once

#include "widget.hpp"
#include "../util/util.hpp"

struct layout_tag {};

struct widget_builder;
struct widget_updater;

template <class Producer, class Consumer, class Destroyer> 
struct view_seq_rebuild_ctx {
  Producer produce;
  Consumer consume;
  Destroyer destroy;
};

struct rebuild_result {
  enum flag {
    none = 0,
    size_change = 1
  };
  
  rebuild_result() : value{flag::none} {}
  rebuild_result(flag f) : value{f} {}
  
  constexpr rebuild_result operator |(rebuild_result o) {
    return {(flag)(value | o.value)};
  }
  
  bool operator&(flag f) const {
    return value & f;
  }
  
  constexpr rebuild_result& operator |=(rebuild_result o) {
    *this = *this | o;
    return *this;
  }
  
  flag value;
};

struct view_sequence_base {
  
};

template <class T>
concept is_view_sequence = is_base_of(^view_sequence_base, ^T);

/// The base for a View. Any View is also a ViewSequence, so we implement this here
template <class T>
struct view : view_sequence_base {
  
  // must declare the following : 
  // build(widget_builder& c, auto& state) -> Widget
  // rebuild(self& new, widget& w, widget_updater up, auto& state)
  
  void seq_build(this T& self, auto Consumer, const widget_builder& c, auto& state) {
    Consumer(self.build(c, state));
  }
  
  rebuild_result seq_rebuild(this T& self, T& New, auto&& seq_updater, 
                   const widget_updater& up, auto& state) 
  {
    return self.rebuild(New, seq_updater.next(), up, state);
  }
  
  void seq_destroy(this T& self, auto GetForDestroy) {
    self.destroy( GetForDestroy() );
  }
};

template <class T>
struct view_sequence_updater {
  
  auto consume_fn(this T& self) {
    return [&self] (auto&& arg) { self.consume((decltype(arg)&&)(arg)); };
  }
  
  auto next_fn(this T& self) {
    return [&self] () -> auto& { return self.next(); };
  }
  
  auto destroy_fn(this T& self) {
    return [&self] () { return self.destroy(); };
  }
};

/// A basic view for a widget 
template <class Widget, class Lens = empty_lens, class Prop = ignore>
struct simple_view_for : view<simple_view_for<Widget, Lens, Prop>>, Prop {
  
  simple_view_for(auto lens = {}) : lens{make_lens(lens)} {}
  
  simple_view_for(Lens lens, Prop prop)
    requires (^Prop != ^ignore)
  : Prop{prop}, lens{lens}
  {
  }
  
  template <class State>
  auto build(const auto& builder, State& state) {
    if constexpr (^Lens == ^empty_lens)
      return Widget{};
    else {
      if constexpr (^Prop != ^ignore)
        return with_lens<State>( Widget{(Prop&)*this}, make_lens(lens) );
      else
        return with_lens<State>( Widget{}, make_lens(lens) );
    }
  }
  
  rebuild_result rebuild(auto&& New, widget_ref w, const auto& updater, auto& state) {
    return {};
    /* if (New.prop == prop)
      return;
    w.template as<Widget>.set_properties(New.prop);
    prop = New.prop; */ 
  }
  
  Lens lens;
};
