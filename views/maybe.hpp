#pragma once

#include "views_core.hpp"

template <class View>
struct maybe : view_sequence_base {
  
  maybe(bool flag, View v) : flag{flag}, view{v} {}
  
  using Self = maybe<View>;
  
  template <class S>
  void seq_build(auto consumer, auto&& b, S& state)
  {
    if (!flag) 
      return;
    view.seq_build(consumer, b, state);
  }
  
  unsigned size() { return flag; }
  
  template <class State>
  void seq_rebuild(Self& New, auto seq_updater, widget_updater& up, State& s) 
  {
    if (flag == New.flag)
    {
      if (flag)
        view.seq_rebuild(New.view, seq_updater, up, s);
    }
    else if (!flag)
    {
      auto nb = up.builder();
      view.seq_build(seq_updater.consume_fn(), nb, s);
      up.parent_widget().layout();
    }
    else
    {
      view.seq_destroy(seq_updater.destroy_fn());
      up.parent_widget().layout();
    }
    flag = New.flag;
  }
  
  bool flag; 
  View view;
};

template <class V>
maybe(bool, V) -> maybe<V>;

/* 
template <class... Ts>
struct either {
  
  template <class L, class S>
  void construct(widget_tree_builder<L>& b, S& state)
  {
    switch(flag)
    {
      
    }
    
    if (!flag) 
      return;
    view.construct(b, state);
  }
  
  unsigned flag;
  tuple<Ts...> children;
}; */ 