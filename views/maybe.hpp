#pragma once

#include "views_core.hpp"

template <class View>
struct maybe {
  
  using Self = maybe<View>;
  
  template <class S>
  void construct(widget_tree_builder& b, S& state)
  {
    if (!flag) 
      return;
    view.construct(b, state);
  }
  
  template <class State>
  void rebuild(Self& New, widget_tree_updater& b, State& s) 
  {
    if (flag == New.flag)
    {
      if (flag)
        view.rebuild(New.view, b, s);
    }
    else if (!flag)
    {
      auto nb = b.builder();
      view.construct(nb, s);
      b.parent_widget()->layout(b.tree);
    }
    else
    {
      b.tree.destroy(b.consume_leaf().id());
      b.parent_widget()->layout(b.tree);
    }
    *this = New;
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