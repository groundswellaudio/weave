#pragma once

#include "views_core.hpp"
#include <ranges>
#include <type_traits>

namespace weave::views {

template <class Range, class ViewCtor>
struct for_each : view_sequence_base {
  
  for_each(auto&& r, ViewCtor ctor) : range{WEAVE_FWD(r)}, view_ctor{ctor} {
    for (auto&& elem : range) {
      elements.push_back(view_ctor(elem));
    }
  }
  
  using element = decltype( std::declval<ViewCtor>()(*std::declval<Range>().begin()) );
  
  void seq_build(auto Consumer, const build_context& b,  auto& state) {
    for (auto&& elem : elements) {
      Consumer( elem.build(b, state) );
    }
  }
  
  rebuild_result seq_rebuild(for_each& Old, auto&& seq_updater, const build_context& ctx, auto& state) 
  {
    // elements were already created by the constructor
    rebuild_result res = {};
    
    if (elements.size() >= Old.elements.size())
    { 
      unsigned k = 0;
      for (; k < Old.elements.size(); ++k)
        res |= elements[k].seq_rebuild(Old.elements[k], seq_updater, ctx, state);
      for (; k < elements.size(); ++k) 
        elements[k].seq_build( seq_updater.consume_fn(), ctx, state );
    }
    else if (Old.elements.size() > elements.size())
    {
      unsigned k = 0;
      for (; k < elements.size(); ++k)
        res |= elements[k].seq_rebuild(Old.elements[k], seq_updater, ctx, state);
      for (; k < Old.elements.size(); ++k)
        Old.elements[k].seq_destroy( seq_updater.destroy_fn(), ctx.application_context() );
    }
    
    return res;
  }
  
  void seq_destroy(auto&& Get, application_context& ctx) {
    for (auto& e : elements)
      e.seq_destroy(Get, ctx);
  }
  
  Range range;
  ViewCtor view_ctor;
  std::vector<element> elements;
};

template <class R, class C>
for_each(R&, C) -> for_each<std::conditional_t<std::ranges::view<R>, R, R&>, C>;

template <class R, class C>
for_each(R&&, C) -> for_each<R, C>;

} // views
