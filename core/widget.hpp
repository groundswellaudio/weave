#pragma once

#include <vector>
#include <iostream>
#include <cassert>
#include <type_traits>
#include <functional>

#include "events/mouse_events.hpp"
#include "events/keyboard.hpp"

#include "../geometry/geometry.hpp"

#include "lens.hpp"
#include "../util/tuple.hpp"
#include "../util/optional.hpp"
#include "../util/variant.hpp"
#include "../util/util.hpp"

namespace weave {

struct painter;
struct event_context;
struct application_context; 

struct input_event : variant<mouse_event, keyboard_event> {
  using variant<mouse_event, keyboard_event>::variant;
  bool is_mouse() const { return index() == 0; }
  auto& mouse() { return get<0>(*this); }
  bool is_keyboard() const { return index() == 1; }
  auto& keyboard() { return get<1>(*this); }
};

struct widget_size_info {
  vec2f min {0, 0};
  vec2f max {infinity<float>(), infinity<float>()};
  // vec2f expand_factor {1, 1};
};

struct layout_context {
  vec2f min{0, 0};
  vec2f max{1000000, 1000000};
};

template <class T>
auto& operator<<(std::ostream& os, const vec<T, 2>& v) {
  os << "{" << v.x << " " << v.y << "}";
  return os;
}

struct widget_ref;

struct any_invocable { bool operator()(auto&&... args) { return false; } };

template <class W>
concept widget_has_children = requires (W& obj) { obj.traverse_children(any_invocable{}); };

struct widget_base {
  
  vec2f size() const { return sz; }
  vec2f position() const { return pos; }
  rectangle area() const { return {pos, sz}; }
  
  void set_size(vec2f v) {
    assert( std::abs(v.x) < 1e10 && std::abs(v.y) < 1e10 && "aberrant size value" );
    assert( !std::isnan(v.x) && !std::isnan(v.y) && "NaN in widget size?" );
    sz = v;
  }
  
  void debug_dump(this auto& self, int indent = 0) {
    for (int k = 0; k < indent; ++k)
      std::cerr << '\t';
    using T = std::remove_reference_t<decltype(self)>;
    auto type_str = stringify<T>();
    if (type_str.starts_with("weave::widgets::"))
      type_str.remove_prefix(sizeof("weave::widgets::") - 1);
    std::cerr << type_str << " " << self.position() << " " << self.size();
    auto info = self.size_info();
    std::cerr << " minmax " << info.min << " " << info.max;
    if constexpr ( widget_has_children<T> ) {
      self.traverse_children( [indent] (auto& elem) {
        std::cerr << '\n';
        elem.debug_dump(indent + 1); 
        return true;
      });
    }
  }
  
  void set_position(float x, float y) {
    set_position({x, y});
  }
  
  void set_position(vec2f p) {
    assert( std::abs(p.x) < 1e10 && std::abs(p.y) < 1e10 && "aberrant position value" );
    assert( !std::isnan(p.x) && !std::isnan(p.y) && "NaN in widget position?" );
    pos = p;
  }
  
  bool contains(vec2f pos) const {
    auto p = position();
    auto sz = size();
    return p.x <= pos.x && p.y <= pos.y && p.x + sz.x >= pos.x && p.y + sz.y >= pos.y;
  }
  
  widget_size_info size_info(this auto& self) {
    return {self.min_size(), self.max_size()};
  }
   
  vec2f do_layout(this auto& self, vec2f sz) {
    if constexpr (requires {self.layout(sz);}) {
      auto res = self.layout(sz);
      self.set_size(res);
      return res;
    }
    else {  
      self.set_size(sz);
      return sz;
    }
  }
  
  std::optional<widget_ref> find_child_at(this auto& self, vec2f pos);
  
  vec2f sz, pos = {0, 0};
};

template <class T>
concept is_child_event_listener = requires (T obj, mouse_event e, widget_ref r, 
                                            event_context& ec)
{
  obj.on_child_event(e, ec, r);
};

template <class T>
concept keyboard_event_listener = requires (T obj, keyboard_event e, event_context& ec) {
  obj.on(e, ec);
};

namespace impl 
{
  using widget_children_vec = std::vector<widget_ref>;
  
  struct widget_vtable {
    template <class T>
    using ptr = T*;
    
    ptr<void(widget_base*, painter&)> paint;
    ptr<vec2f(widget_base*, vec2f sz)> layout;
    ptr<widget_size_info(const widget_base*)> size_info;
    ptr<void(widget_base*, input_event, event_context&)> on;
    ptr<void(widget_base*, input_event, event_context&, widget_ref)> on_child_event;
    ptr<optional<widget_ref>(widget_base*, vec2f)> find_child_at;
    ptr<void(widget_base*, widget_children_vec&)> children;
    ptr<void(widget_base*, int indent)> debug_dump;
    ptr<void(widget_base*)> destroy;
  };
  
  template <class T>
  struct widget_vtable_impl;
} // impl

class widget_ref {
  
  friend struct widget_box;
  
  using children_vec = impl::widget_children_vec;
  using vtable = impl::widget_vtable;
  
  widget_base* data;
  const vtable* vptr;
  
  widget_ref(widget_base* data, const vtable* vptr) : data{data}, vptr{vptr} {} 
  
  public :
  
  widget_ref() : data{nullptr} {}
  
  template <class W>
    requires (std::is_base_of_v<widget_base, W>)
  widget_ref(W* widget) {
    data = widget;               
    vptr = &impl::widget_vtable_impl<std::remove_const_t<W>>::value;
  }
  
  widget_ref(const widget_ref& o) noexcept {
    data = o.data;
    vptr = o.vptr;
  }
  
  auto& operator=(widget_ref o) noexcept {
    data = o.data;
    vptr = o.vptr;
    return *this;
  }
  
  bool operator==(widget_ref o) const {
    return data == o.data;
  }
  
  auto layout(vec2f sz) {
    return vptr->layout(data, sz);
  }

  template <class T>
  T& as() {
    assert( is<T>() && "widget is not of the requested type" ); 
    return *static_cast<T*>(data); 
  }
  
  template <class T>
  const T& as() const { 
    assert( is<T>() && "widget is not of the requested type" ); 
    return *static_cast<T*>(data); 
  }
  
  template <class T>
    requires (std::is_base_of_v<widget_base, T>)
  bool is() const {
    return vptr == &impl::widget_vtable_impl<std::remove_const_t<T>>::value;
  }
  
  void paint(painter& p) const {
    vptr->paint(data, p);
  }

  void on(input_event e, event_context& ec) {
    vptr->on(data, e, ec);
  }
  
  void on_child_event(input_event e, event_context& ec, widget_ref child) {
    if (vptr->on_child_event)
      vptr->on_child_event(data, e, ec, child);
  }
  
  bool is_child_event_listener() const { return vptr->on_child_event; }
  
  widget_size_info size_info() const { return vptr->size_info(data); }
  
  vec2f size() const { return data->size(); }
  rectangle area() const { return data->area(); }
  
  void set_size(vec2f sz) { data->set_size(sz); }
  
  vec2f position() const { return data->position(); }
  
  void set_position(vec2f p) { data->set_position(p.x, p.y); }
  
  bool contains(vec2f pos) const { return data->contains(pos); }
  
  auto find_child_at(vec2f pos) const {
    return vptr->find_child_at(data, pos);
  }
  
  std::vector<widget_ref> children() const {
    std::vector<widget_ref> vec; 
    vptr->children(data, vec); 
    return vec;
  }
  
  void debug_dump(int indent = 0) const {
    vptr->debug_dump(data, indent);
  }
};

struct widget_box : widget_ref {
  
  widget_box(std::nullptr_t) : widget_ref{nullptr, nullptr} {}
  
  template <class W>
    requires (std::is_base_of_v<widget_base, std::remove_reference_t<W>>)
  widget_box(W&& widget) {
    data = new std::decay_t<W> { (W&&)widget };
    vptr = &impl::widget_vtable_impl<std::decay_t<W>>::value;
  }
  
  widget_box(widget_box&& o) noexcept {
    data = o.data;
    vptr = o.vptr;
    o.data = nullptr;
  }
  
  auto& operator=(widget_box&& o) noexcept {
    data = o.data;
    vptr = o.vptr;
    o.data = nullptr;
    return *this;
  }
  
  operator bool () const {
    return data;
  }
  
  widget_ref borrow() {
    return widget_ref{data, vptr};
  }
  
  void reset() {
    if (data) {
      vptr->destroy(data);
      data = nullptr;
    }
  }
  
  ~widget_box() { if (data) vptr->destroy(data); }
};

namespace impl {
  
  template <class T>
    requires (std::is_base_of_v<widget_base, std::remove_reference_t<T>>)
  widget_ref to_widget_ref(T&& val) {
    return widget_ref{&val};
  }
  
  widget_ref to_widget_ref(widget_box& box) {
    return box.borrow();
  }
  
} // impl

optional<widget_ref> widget_base::find_child_at(this auto& self, vec2f pos) {
  if constexpr ( widget_has_children<decltype(self)> ) {
    optional<widget_ref> res;
    self.traverse_children( [&res, pos] (auto& elem) {
      return !elem.contains(pos) || (res = impl::to_widget_ref(elem), false);
    });
    return res;
  }
  else {
    return {};
  }
}

/// A stack of parents [p+n...p0] which is constructed on traversal
/// and provided by event_context
using event_context_parent_stack = std::vector<widget_ref>;

struct event_frame_result {
  bool rebuild_requested = false;
  bool repaint_requested = false;
};

struct event_context {

  /// Returns a thread safe callable that signal that a rebuild is needed
  auto lift_rebuild_request();
  
  void request_rebuild() { frame_result.rebuild_requested = true; }
  void request_repaint() { frame_result.repaint_requested = true; }
  
  widget_ref parent() const { return parents.back(); }
  
  void push_overlay(widget_box widget);
  
  void push_overlay_relative(widget_box widget);
  
  void pop_overlay();
  
  /// Register an animation fn to be executed every period_in_ms
  template <class W, class Fn>
  void animate(W& widget, Fn fn, int period_in_ms);
  
  /// Remove all animations
  void deanimate(widget_ref w);
  
  vec2f absolute_position() const {
    vec2f res {0, 0};
    for (auto p : parents)
      res += p.position();
    return res;
  }
  
  void grab_keyboard_focus(widget_ref r);
  
  void release_keyboard_focus();
  
  widget_ref current_mouse_focus();
  
  void grab_mouse_focus(widget_ref w);
  
  void reset_mouse_focus();
  
  auto& context() const { return ctx; }
  
  template <class S>
  S& state() const { return *static_cast<S*>(state_ptr); }
  
  event_context with_parent(widget_ref p) const {
    auto Res {*this};
    Res.parents.push_back(p);
    return Res;
  }
  
  /* 
  template <class S, class View>
  auto build_view(View&& v) {
    auto w = v.build( build_context{context()}, state<S>() );
    return w;
  } */ 
  
  application_context& ctx;
  event_frame_result& frame_result;
  event_context_parent_stack parents;
  void* state_ptr;
};

namespace impl {
  
  template <class W>
  consteval auto child_event_fn_ptr() {
    if constexpr ( is_child_event_listener<W> )
      return + [] (widget_base* self, input_event e, event_context& ec, widget_ref child) {
        auto& Obj = *static_cast<W*>(self);
        if (e.index() == 0)
          Obj.on_child_event(get<0>(e), ec, child);
        else if constexpr ( requires { Obj.on_child_event(get<1>(e), ec, child); } )
          Obj.on_child_event(get<1>(e), ec, child);
      };
    else
      return nullptr;
  }
  
  template <class W>
  struct widget_vtable_impl 
  {
    static constexpr widget_vtable value = {
      +[] (widget_base* self, painter& p) {
        auto& obj = *static_cast<W*>(self);
        obj.paint(p);
      },
      +[] (widget_base* self, vec2f sz) -> vec2f {
        auto& obj = *static_cast<W*>(self);
        return obj.do_layout(sz);
      },
      +[] (const widget_base* self) -> widget_size_info {
        return static_cast<const W*>(self)->size_info();
      },
      +[] (widget_base* self, input_event e, event_context& ctx) {
        auto& Obj = *static_cast<W*>(self);
        if (e.index() == 0) {
          return Obj.on(get<0>(e), ctx);
        }
        else {
          // reacting to keyboard event is optional
          if constexpr ( requires { Obj.on(get<1>(e), ctx); } )
            return Obj.on(get<1>(e), ctx);
        }
      },
      child_event_fn_ptr<W>(),
      +[] (widget_base* self, vec2f pos) -> optional<widget_ref> {
        auto& obj = *static_cast<W*>(self);
        return obj.find_child_at(pos);
      },
      +[] (widget_base* self, widget_children_vec& vec) {
        if constexpr ( widget_has_children<W> )
          static_cast<W*>(self)->traverse_children( [&vec] (auto&& elem) {
            vec.push_back(to_widget_ref(elem));
            return true;
          });
      },
      +[] (widget_base* self, int indent) {
        static_cast<W*>(self)->debug_dump(indent + 1);
      },
      +[] (widget_base* self) {
        delete static_cast<W*>(self);
      }
    };
  };
}

} // weave