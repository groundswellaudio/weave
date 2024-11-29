#pragma once

#include <vector>
#include <iostream>
#include <cassert>
#include <type_traits>

#include "events/mouse_events.hpp"
#include "events/keyboard.hpp"

#include "lens.hpp"
#include "../util/tuple.hpp"
#include "../util/optional.hpp"
#include "../util/meta.hpp"
#include "../util/variant.hpp"

struct painter;
struct widget_builder;
struct widget_updater;

using input_event = variant<mouse_event, keyboard_event>;

consteval std::meta::expr to_str(std::meta::type t) {
  std::meta::ostream os;
  os << t;
  return make_literal_expr(os);
}

template <class T>
auto& operator<<(std::ostream& os, const vec<T, 2>& v) {
  os << "{" << v.x << " " << v.y << "}";
  return os;
}

struct widget_ref;

struct widget_base {
  
  vec2f size() const { return sz; }
  vec2f position() const { return pos; }
  
  void set_size(vec2f v) {
    sz = v;
  }
  
  void set_position(float x, float y) {
    pos = vec2f{x, y};
  }
  
  void set_position(vec2f p) {
    pos = p;
  }
  
  bool contains(vec2f pos) {
    auto p = position();
    auto sz = size();
    return p.x <= pos.x && p.y <= pos.y && p.x + sz.x >= pos.x && p.y + sz.y >= pos.y;
  }
  
  std::optional<widget_ref> find_child_at(this auto& self, vec2f pos);
  
  vec2f sz, pos = {0, 0};
};

namespace impl {
  template <class L, class T>
  constexpr auto dyn_lens_write_ptr() {
    if constexpr ( !is_reference(^T)
      && requires (L obj, typename L::input state, T val) { obj.write(state, val); } ) 
    {
      return + [] (void* data, void* state, T val) {
        static_cast<L*>(data)->write(*static_cast<typename L::input*>(state), val);
      };
    }
    else
      return nullptr;
  }
}

template <class T>
class dyn_lens {
  
  struct vtable_t {
    void*(*clone)(void* data);
    T(*read)(void*, void*);
    void(*write)(void*, void*, T);
    void(*destroy)(void*);
  };
  
  template <class L>
  static constexpr vtable_t vtable_for {
    +[] (void* data) -> void* {
      return new L {*static_cast<L*>(data)};
    },
    +[] (void* data, void* state) -> T {
      return static_cast<L*>(data)->read(*static_cast<typename L::input*>(state));
    },
    impl::dyn_lens_write_ptr<L, T>(),
    + [] (void* data) {
      delete static_cast<L*>(data);
    }
  };
  
  public : 
  
  template <class L>
  dyn_lens(L lens) { emplace(lens); }
  
  dyn_lens(dyn_lens&& o) noexcept {
    data = o.data;
    vptr = o.vptr;
    o.data = nullptr;
  }
  
  template <class L>
  void emplace(L lens) {
    vptr = &vtable_for<L>;
    data = new L {lens};
  }
  
  dyn_lens<T> clone() const {
    return {vptr->clone(data), vptr};
  }
  
  T read(void* state) const { return vptr->read(data, state); }
  
  void write(void* state, T val) const 
  requires (!is_reference(^T)) 
  { 
    vptr->write(data, state, val); 
  }
  
  ~dyn_lens() {
    if (data)
      vptr->destroy(data);
  }
  
  private : 
  
  dyn_lens(void* data, const vtable_t* vptr) : data{data}, vptr{vptr} 
  {
  }
  
  void* data;
  const vtable_t* vptr;  
};

template <class W>
using dyn_lens_for = dyn_lens<typename W::value_type>;

template <class T>
struct event_context_t;

using event_context_data = event_context_t<void>;

template <class W>
using event_context = event_context_t<typename W::value_type>;

template <class T>
concept is_child_event_listener = requires (T obj, mouse_event e, widget_ref r, event_context<T> ec)
{
  obj.on_child_event(e, ec, r);
};

template <class T>
concept keyboard_event_listener = requires (T obj, keyboard_event e, event_context<T> ec) {
  obj.on(e, ec);
};

struct any_invocable { bool operator()(auto&& elem) { return false; } };

template <class W>
concept widget_has_children = requires (W& obj) { obj.traverse_children(any_invocable{}); };

template <class W>
concept widget_has_interface = ^typename W::value_type != ^void;

namespace impl 
{
  using widget_children_vec = std::vector<widget_ref>;
  
  struct widget_vtable {
    template <class T>
    using ptr = T*;
    
    ptr<void(widget_base*, painter&, void* state)> paint;
    ptr<vec2f(widget_base*)> layout;
    ptr<void(widget_base*, input_event, event_context_data)> on;
    ptr<void(widget_base*, input_event, event_context_data, widget_ref)> on_child_event;
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
    requires (is_base_of(^widget_base, ^W))
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
  
  auto layout() {
    return vptr->layout(data);
  }

  template <class T>
  T& as() { return *static_cast<T*>(data); }
  
  template <class T>
  const T& as() const { return *static_cast<T*>(data); }
  
  template <class T>
    requires (is_base_of(^widget_base, ^T))
  bool is() const {
    return vptr == &impl::widget_vtable_impl<std::remove_const_t<T>>::value;
  }
  
  void paint(painter& p, void* state) const {
    vptr->paint(data, p, state);
  }

  void on(input_event e, event_context_data ec);
  
  void on_child_event(input_event e, event_context_data ec, widget_ref child);
  
  bool is_child_event_listener() const { return vptr->on_child_event; }
  
  vec2f size() const { return data->size(); }
  
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
    requires (is_base_of(^widget_base, remove_reference(^W)))
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
    requires (is_base_of(^widget_base, remove_reference(^T)))
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

template <class T>
struct event_context_t;

struct application_context; 

struct event_context_base {
  
  template <class W, class P>
  void open_modal_menu(this auto& self, W&& widget, P* parent);
  
  void close_modal_menu();
  
  void grab_keyboard_focus(this auto& self, widget_ref r);
  
  void release_keyboard_focus();
  
  widget_ref current_mouse_focus();
  
  void reset_mouse_focus();
  
  auto& context() const { return ctx; }
  
  application_context& ctx;
};

template <> 
struct event_context_t<void> : event_context_base {
  
  void* state() { return state_ptr; }
  widget_ref parent() const { return parents.back(); }
  
  event_context_parent_stack parents;
  void* state_ptr;
};

using event_context_data = event_context_t<void>;

template <class T>
struct event_context_t : event_context_base 
{
  void write(T v) 
    requires (!is_reference(^T))
  {
    lens.write(state_ptr, v);
  }

  T read() const {
    return lens.read(state_ptr);
  }
  
  const event_context_parent_stack& parents;
  void* state_ptr;
  const dyn_lens<T>& lens;
};

template <class T>
event_context_t<T> apply_lens(const event_context_data& ec, const dyn_lens<T>& lens) {
  return {
    {ec.ctx}, 
    ec.parents,
    ec.state_ptr,
    lens
  };
}

void widget_ref::on(input_event e, event_context_data ec) {
  vptr->on(data, e, ec);
}

void widget_ref::on_child_event(input_event e, event_context_data ec, widget_ref child) {
  if (vptr->on_child_event)
    vptr->on_child_event(data, e, ec, child);
}

template <class W>
struct with_lens_t;

namespace impl {
  
  template <class W>
  consteval auto child_event_fn_ptr() {
    if constexpr ( is_child_event_listener<W> )
      return + [] (widget_base* self, input_event e, event_context_data data, widget_ref child) {
        auto& Obj = *static_cast<W*>(self);
        if (e.index() == 0)
          Obj.on_child_event(get<0>(e), data, child);
        else if constexpr ( requires { Obj.on_child_event(get<1>(e), data, child); } )
          Obj.on_child_event(get<1>(e), data, child);
      };
    else
      return nullptr;
  }
  
  template <class W>
  struct widget_vtable_impl 
  {
    static constexpr widget_vtable value = {
      +[] (widget_base* self, painter& p, void* state) {
        auto& obj = *static_cast<W*>(self);
        if constexpr ( requires {obj.paint(p, state);} )
          obj.paint(p, state);
        else
          obj.paint(p);
      },
      +[] (widget_base* self) -> vec2f {
        auto& obj = *static_cast<W*>(self);
        if constexpr ( requires {obj.layout();} ) {
          auto sz = obj.layout();
          obj.set_size(sz);
          return sz;
        }
        else
          return obj.size();
      },
      +[] (widget_base* self, input_event e, event_context_data ctx) {
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
        for (int k = 0; k < indent; ++k)
          std::cerr << '\t';
        std::cerr << %stringify(^W) << " " << self->position() << " " << self->size();
        if constexpr ( widget_has_children<W> ) {
          static_cast<W*>(self)->traverse_children( [indent] (auto& elem) {
            std::cerr << '\n';
            to_widget_ref(elem).debug_dump(indent+1);
            return true;
          });
        }
      },
      +[] (widget_base* self) {
        delete static_cast<W*>(self);
      }
    };
  };
  
  // Important : the vtable of a widget that must have a lens is the vtable 
  // of with_lens(T)
  template <class W>
    requires (!std::is_same_v<typename W::value_type, void>)
  struct widget_vtable_impl<W> 
    : widget_vtable_impl<with_lens_t<W>> 
  {
  };
  
}

template <class Fn, class State>
struct invocable_lens {
  using input = State;
  
  decltype(auto) read(State& s) {
    auto _ = s.read_scope();
    return (fn(s));
  }
  
  void write(State& s, auto&& val) 
  {
    auto _ = s.write_scope();
    fn(s) = (decltype(val)&&)val;
  }
  
  Fn fn;
};

template <class ValueType, class State, class Lens>
auto make_lens(Lens lens) {
  if constexpr ( requires (State& s) { lens(s); } )
    return dyn_lens<ValueType>{invocable_lens<Lens, State>{lens}};
}

/// A widget along its state lens
template <class W>
struct with_lens_t : W {
  
  using value_type = void;
  
  void on(mouse_event e, event_context_data ctx) {
    event_context<W> ev_ctx = apply_lens(ctx, lens);
    W::on(e, ev_ctx);
  }
  
  void on(keyboard_event e, event_context_data ctx) 
    requires keyboard_event_listener<W>
  {
    event_context<W> ev_ctx = apply_lens(ctx, lens);
    W::on(e, ev_ctx);
  }
  
  void on_child_event(input_event e, event_context_data ctx, widget_ref child) 
    requires ( is_child_event_listener<W> )
  {
    event_context<W> ev_ctx = apply_lens(ctx, lens);
    if (e.index() == 0)
      W::on_child_event(get<0>(e), ev_ctx, child);
    else if constexpr ( requires {W::on_child_event(get<1>(e), ev_ctx, child);} )
      W::on_child_event(get<1>(e), ev_ctx, child);
  }
  
  void paint(painter& p, void* state) {
    W::paint(p, lens.read(state));
  }
  
  dyn_lens_for<W> lens;
};

template <class W, class Lens>
with_lens_t(W w, Lens lens) -> with_lens_t<W>;

template <class State, class W, class Lens>
auto with_lens(W&& widget, Lens lens) {
  return with_lens_t{ (decltype(widget)&&)widget, 
                       make_lens<typename W::value_type, State>(lens) };
}

struct application_context;

struct widget_builder 
{
  auto& context() const { return ctx; }
  application_context& ctx;
};

struct widget_updater 
{
  widget_builder builder() const { return {ctx}; }
  auto& context() const { return ctx; }
  application_context& ctx;
};



/* 
struct view_archetype : view {
  
  widget_tree::widget* construct(widget_tree_builder& b);
    // { return b.tree.construct_widget(id); }
}; */ 