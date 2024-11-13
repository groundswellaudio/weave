#pragma once

#include "views_core.hpp"
#include <utility>

template <class State, class RangeFn, class ViewCtor>
auto for_each_ctor_res(State& state, RangeFn range_fn, ViewCtor ctor) 
  -> decltype( ctor( compose_lens(range_fn, 
                                       range_element_lens<std::remove_reference_t<decltype(range_fn(state))>>{0}), 
                      state ) );
 
template <class Range, class ViewCtor>
struct for_each {
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
  void rebuild(Self& NewView, widget_tree_updater& b, State& state)
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
      unsigned k = 0;
      for (; k < elements.size(); ++k)
        elements[k].rebuild(NewView.elements[k], b, state);
      for (; k < NewView.elements.size(); ++k) {
        elements.emplace_back(NewView.elements[k]);
        auto next_b = b.builder();
        elements.back().construct(next_b, state);
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