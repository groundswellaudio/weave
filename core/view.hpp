#pragma once

#include "widget.hpp"
#include "../util/util.hpp"

namespace weave {

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

struct application_context; 
struct graphics_context;

struct build_context 
{
  graphics_context& graphics_context() const;
  auto& context() const { return ctx; }
  
  application_context& ctx;
};

template <class T>
concept is_view_sequence = std::is_base_of_v<view_sequence_base, T>;

struct view_base {};

/// The base for a View. Any View is also a ViewSequence, so we implement this here
template <class T>
struct view : view_sequence_base, view_base {
  
  // must declare the following : 
  // build(build_context& c, auto& state) -> Widget
  // rebuild(self& old, widget_ref w, build_context ctx, auto& state) -> rebuild_result
  
  void seq_build(this auto& self, auto Consumer, const build_context& ctx, auto& state) {
    Consumer(self.build(ctx, state));
  }
  
  template <class O>
  rebuild_result seq_rebuild(this O& self, O& old, auto&& seq_updater, 
                   const build_context& ctx, auto& state) 
  {
    return self.rebuild(old, seq_updater.next(), ctx, state);
  }
  
  void seq_destroy(this auto& self, auto GetForDestroy, application_context& ctx) {
    self.destroy( GetForDestroy(), ctx );
  }
  
  void destroy(widget_ref w, application_context& ctx) {}
};

template <class T>
concept is_view = std::is_base_of_v<view_base, T>;

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
    requires (not std::is_same_v<Prop, ignore>)
  : Prop{prop}, lens{lens}
  {
  }
  
  template <class State>
  auto build(const auto& builder, State& state) {
    if constexpr (not std::is_same_v<Prop, ignore>)
      return Widget{(Prop&)*this};
    else
      return Widget{};
  }
  
  rebuild_result rebuild(auto&& New, widget_ref w, const auto& updater, auto& state) {
    return {};
  }
  
  Lens lens;
};

} // weave
