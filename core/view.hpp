#pragma once

#include "widget.hpp"

struct layout_tag {};

struct widget_builder;
struct widget_updater;

template <class Producer, class Consumer, class Destroyer> 
struct view_seq_rebuild_ctx {
  Producer produce;
  Consumer consume;
  Destroyer destroy;
};

/// The base for a View. Any View is also a ViewSequence, so we implement this here
template <class T>
struct view {
  
  // must declare the following : 
  // build(widget_builder& c, auto& state) -> Widget
  // rebuild(self& new, widget& w, widget_updater up, auto& state)
  
  void seq_build(this T& self, auto Consumer, const widget_builder& c, auto& state) {
    Consumer(self.build(c, state));
  }
  
  void seq_rebuild(this T& self, T& New, auto&& seq_updater, 
                   const widget_updater& up, auto& state) 
  {
    self.rebuild(New, seq_updater.next(), up, state);
  }
  
  void seq_destroy(this T& self, auto GetForDestroy) {
    self.destroy( GetForDestroy() );
  }
  
  void seq_destroy(this T& self, widget_ref ref) {
    self.destroy(ref);
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
    return [&self] () -> auto& { return self.destroy(); };
  }
};