#pragma once

#include "ui_tree.hpp"

struct layout_tag {};

struct widget_builder;
struct widget_updater;

template <class Producer, class Consumer, class Destroyer> 
struct view_seq_rebuild_ctx {
  Producer produce;
  Consumer consume;
  Destroyer destroy;
};

template <class T>
struct view {
  
  // must declare the following : 
  // build(widget_builder& c, auto& state) -> tuple(widget, lens, widget_ctor_args)
  // rebuild(self& new, widget& w, widget_updater up, auto& state)
  
  void seq_build(this T& self, auto Consumer, const widget_builder& c, auto& state) {
    auto [w, lens, args] = self.build(c, state);
    Consumer(std::move(w), lens, args);
  }
  
  void seq_rebuild(this T& self, T& New, auto&& seq_updater, 
                   const widget_updater& up, auto& state) 
  {
    self.rebuild(New, seq_updater.next(), up, state);
  }
  
  void seq_destroy(this T& self, auto GetForDestroy, widget_tree& tree) {
    self.destroy( GetForDestroy(), tree );
  }
  
  void destroy(widget& w, widget_tree& t) {
    t.destroy(w.id());
  }
};

template <class T>
struct view_sequence_updater {
  
  auto consume_fn(this T& self) {
    return [&self] (auto&&... args) { self.consume(args...); };
  }
  
  auto next_fn(this T& self) {
    return [&self] () -> auto& { return self.next(); };
  }
  
  auto destroy_fn(this T& self) {
    return [&self] () -> auto& { return self.destroy(); };
  }
};