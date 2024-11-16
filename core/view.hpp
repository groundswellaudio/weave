#pragma once

#include <functional>

struct layout_tag {};

struct view_id {
  bool operator==(const view_id&) const = default;
  unsigned value = 0;
};

struct widget_id {
  
  constexpr widget_id() = default;
  constexpr widget_id(view_id id) : base{id} {}
  
  bool operator==(const widget_id& o) const = default; 
  
  static constexpr widget_id root() { return {view_id{0}}; }
  
  view_id base;
  unsigned offset = 0;
};

template <>
class std::hash<widget_id> {
  public : 
  constexpr std::size_t operator()(const widget_id& x) const { 
    auto base = std::size_t(x.base.value) << 32;
    return base | std::size_t(x.offset); 
  }
};

struct view {
  auto& operator=(const view&) { return *this; }
  bool operator==(const view& o) const { return true; }
  bool operator!=(const view& o) const { return false; }
  widget_id this_id = {}; 
};

// A base class for a view that has children
struct composed_view : view {};

template <class T>
struct view_trait;

template <class T>
struct view_sequence_trait;

template <class T>
  requires (has_view_trait<T>)
struct view_sequence_trait {
  
  void build_seq( 
};