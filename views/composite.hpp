#pragma once

#include "views_core.hpp"

#include <cassert>

namespace weave::views {

/// A composite view is a composition of a view state and a body() function 
/// which is a product of both this view state and the global state.
template <class T, class State>
struct composite_view : view<composite_view<T, State>> {
  
  using view_state_t = typename T::view_state;
  
  using body_t = decltype(std::declval<T&>().body(std::declval<State&>(), std::declval<view_state_t&>()));
  
  composite_view(auto&&... args) : definition{args...} {}
  composite_view(composite_view&&) = default;
  
  struct Data {
    view_state_t view_state;
    optional<body_t> definition_body;
  };
  
  auto build(const build_context& ctx, State& state) {
    auto ptr = new Data {definition.init_state()};
    ptr->definition_body.emplace(definition.body(state, ptr->view_state));
    data.reset(ptr);
    return data->definition_body->build(ctx, state);
  }
  
  auto rebuild(auto& old, widget_ref r, auto&& up, auto& state) {
    // Move the value
    assert( old.data.get() && "no data pointer?" );
    auto old_body = std::move(old.data->definition_body);
    // Move the pointer
    data = std::move(old.data);
    data->definition_body.emplace(definition.body(state, data->view_state));
    return data->definition_body->rebuild(*old_body, r, up, state);
  }
  
  T definition;
  std::unique_ptr<Data> data;
};

} // views