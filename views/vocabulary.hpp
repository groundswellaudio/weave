#pragma once

#include "views_core.hpp"
#include <utility>

template <class State, class RangeFn, class ViewCtor>
auto for_each_ctor_res(State& state, RangeFn range_fn, ViewCtor ctor) 
  -> decltype( ctor( compose_lens(range_fn, 
                                       range_element_lens<std::remove_reference_t<decltype(range_fn(state))>>{0}), 
                      state ) );
 
template <class Range, class ViewCtor>
struct for_each : view {
  Range range_fn;
  ViewCtor view_ctor;
  
  using Self = for_each<Range, ViewCtor>;
  
  using element = decltype(for_each_ctor_res(std::declval<typename Range::input&>(), std::declval<Range>(), std::declval<ViewCtor>()));
  
  std::vector<element> elements;
  
  template <class S>
  auto construct(widget_tree_builder& b, S& state)
  {
    int k = 0;
    auto&& range_data = range_fn(state);
    for (auto&& data : range_data)
    {
      using input_t = %remove_reference(type_of(^range_data));
      auto lens = compose_lens(range_fn, range_element_lens<input_t>{k++});
      auto view = view_ctor(lens, state);
      view.construct(b, state);
      elements.push_back(view);
    }
  }
  
  template <class State>
  void rebuild(Self& NewView, widget_tree_builder& b, State& state)
  {
    int k = 0;
    auto&& range_data = range_fn(state);
    for (auto e : range_fn(state))
    {
      using input_t = %remove_reference(type_of(^range_data));
      auto lens = compose_lens(range_fn, range_element_lens<input_t>{k++});
      NewView.elements.push_back(view_ctor(lens, state));
    }
    
    if (NewView.elements.size() > elements.size())
    { 
      b.set_construct_after(view::this_id);
      unsigned k = 0;
      for (; k < elements.size(); ++k)
        elements[k].rebuild(NewView.elements[k], b, state);
      for (; k < NewView.elements.size(); ++k) {
        elements.emplace_back(NewView.elements[k]);
        elements.back().construct(b, state);
      }
      
      b.parent_widget()->layout(b.tree);
    }
    
    /* 
    if (NewView.elements.size() <= elements.size())
    {
      for (unsigned k = 0; k < NewView.elements.size(); ++k)
      {
        elements[k].rebuild(NewView.elements[k], tree);
      }
      for (unsigned k = NewView.elements.size(); k < elements.size(); ++k)
      {
        elements[k].destroy(tree);
      }
      
      elements.remove(elements.begin() + NewView.elements.size(), elements.end());
    }
    else
    {
      int k = 0;
      for (auto& e : elements)
        e.rebuild(NewView.elements[k++], tree);
      for (k < NewView.elements.size(); ++k)
        NewView.elements[k].construct( tree.builder() );
    } */ 
  }
};

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
  void rebuild(Self& New, widget_tree_builder& b, State& s) 
  {
    if (flag == New.flag)
    {
      if (flag)
        view.rebuild(New.view, b, s);
    }
    else if (!flag)
    {
      view.construct(b, s);
      b.parent_widget()->layout(b.tree);
    }
    else
      view.destroy(b.tree);
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