#pragma once

#include "views_core.hpp"

template <class Range, class ViewCtor>
struct for_each : view_sequence_base {
  
  for_each(auto&& range, ViewCtor ctor) : range{range}, view_ctor{ctor} {}
  
  using element = decltype( std::declval<ViewCtor>()(std::declval<Range>().front()) );
   
  void seq_build(auto Consumer, const widget_builder& b,  auto& state) {
    for (auto elem : range) {
      elements.push_back(view_ctor(elem));
      Consumer( elements.back().build(b, state) );
    }
  }
  
  rebuild_result seq_rebuild(for_each& Old, auto&& seq_updater, const widget_updater& up, auto& state) 
  {
    for (auto e : range) {
      elements.push_back(view_ctor(e));
    }
    
    rebuild_result res = {};
    
    if (elements.size() >= Old.elements.size())
    { 
      unsigned k = 0;
      for (; k < Old.elements.size(); ++k)
        res |= elements[k].seq_rebuild(Old.elements[k], seq_updater, up, state);
      for (; k < elements.size(); ++k) 
        elements[k].seq_build( seq_updater.consume_fn(), up.builder(), state );
    }
    
    return res;
  }
  
  Range range;
  ViewCtor view_ctor;
  std::vector<element> elements;
};

template <class R, class V>
for_each(R&, V) -> for_each<R&, V>;

template <class R, class V>
for_each(R&&, V) -> for_each<R, V>;



