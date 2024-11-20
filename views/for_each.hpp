#pragma once

#include "views_core.hpp"
#include <ranges>

template <class Range, class ViewCtor>
struct for_each : view_sequence_base {
  
  simple_for_each(auto&& range, ViewCtor ctor) : range{range}, view_ctor{ctor} {}
  
  using element = decltype( std::declval<ViewCtor>()(std::declval<Range>().front()) );
   
  void seq_build(auto Consumer, const widget_builder& b,  auto& state) {
    for (auto elem : range) {
      elements.push_back(view_ctor(elem));
      Consumer( elements.back().build(b, state) );
    }
  }
  
  void seq_rebuild(simple_for_each& NewView, auto&& seq_updater, widget_updater& up, auto& state) 
  {
    for (auto e : NewView.range) {
      NewView.elements.push_back(view_ctor(e));
    }
    
    if (NewView.elements.size() > elements.size())
    { 
      unsigned k = 0;
      for (; k < elements.size(); ++k)
        elements[k].seq_rebuild(NewView.elements[k], seq_updater, up, state);
      for (; k < NewView.elements.size(); ++k) {
        elements.emplace_back(NewView.elements[k]);
        elements.back().seq_build( seq_updater.consume_fn(), 
                                   up.builder(), state );
      }
      
      up.parent_widget().layout();
    }
  }
  
  Range range;
  ViewCtor view_ctor;
  std::vector<element> elements;
};

template <class R, class V>
simple_for_each(R&, V) -> simple_for_each<R&, V>;

template <class R, class V>
simple_for_each(R&&, V) -> simple_for_each<R, V>;

inline auto iota(int k) {
  return std::ranges::iota_view(0, k);
}



