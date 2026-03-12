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
struct widget_tree;
struct event_context;
struct application_context;
struct destroy_context; 

struct keyboard_focus_release {};

struct input_event : variant<mouse_event, keyboard_event, keyboard_focus_release> {
  using variant<mouse_event, keyboard_event, keyboard_focus_release>::variant;
  bool is_mouse() const { return index() == 0; }
  auto& mouse() { return get<0>(*this); }
  bool is_keyboard() const { return index() == 1; }
  auto& keyboard() { return get<1>(*this); }
};

struct widget_size_info {
  point nominal {0, 0};
  point min {0, 0};
  point max {infinity<float>(), infinity<float>()};
  point flex_factor{1, 1};
  optional<float> aspect_ratio = {}; // the ratio of width / height 
};

struct widget_ref;

struct any_invocable { bool operator()(auto&&... args) { return false; } };

template <class W>
concept widget_has_children = requires (W& obj) { obj.traverse_children(any_invocable{}); };

struct widget_id {
  
  friend struct widget_tree;
  
  widget_id(const widget_id& o) : value{o.value} {}
  
  widget_id& operator=(widget_id o) {
    value = o.value;
    return *this;
  }
  
  bool operator==(const widget_id& o) const {
    return value == o.value;
  }
  
  unsigned raw() const { return value; }
  
  private : 
  
  widget_id(unsigned x) : value{x} {}
  
  unsigned value; 
};

} // weave

template <>
struct std::hash<weave::widget_id> {
  std::size_t operator()(weave::widget_id id) const {
    return std::hash<unsigned>{}(id.raw());
  }
};

namespace weave {

struct destroy_context {
  struct widget_tree& tree() const { return widget_tree; }
  struct widget_tree& widget_tree;
};

struct widget_base {
  
  point size() const { return sz; }
  
  point position() const { return pos; }
  
  point absolute_position(const widget_tree& tree) const;
  
  rectangle area() const { return {pos, sz}; }
  
  void set_size(point v) {
    assert( std::abs(v.x) < 1e10 && std::abs(v.y) < 1e10 && "aberrant size value" );
    assert( v.x >= 0 && v.y >= 0 && "negative size value" );
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
    std::cerr << " min " << info.min << " max " << info.max << " flex " << info.flex_factor
    << " nominal_size " << info.nominal;
    self.traverse_children( [indent] (auto& elem) {
      std::cerr << '\n';
      elem.debug_dump(indent + 1); 
      return true;
    });
  }
  
  void set_position(float x, float y) {
    set_position({x, y});
  }
  
  void set_position(point p) {
    assert( std::abs(p.x) < 1e10 && std::abs(p.y) < 1e10 && "aberrant position value" );
    assert( !std::isnan(p.x) && !std::isnan(p.y) && "NaN in widget position?" );
    pos = p;
  }
  
  bool contains(point pos) const {
    auto p = position();
    auto sz = size();
    return p.x <= pos.x && p.y <= pos.y && p.x + sz.x >= pos.x && p.y + sz.y >= pos.y;
  }
   
  void do_layout(this auto& self, point sz) {
    if constexpr (requires {self.layout(sz);}) {
      self.layout(sz);
    }
    self.set_size(sz);
  }
  
  void on_keyboard_focus_release(event_context& ec) {}
  
  std::optional<widget_ref> find_child_at(this auto& self, point pos);
  
  void destroy(this auto&& self, destroy_context ctx);
  
  void mount(this auto& self, widget_tree& tree, widget_id parent);
  
  void unmount(this auto& self, widget_tree&);
  
  void traverse_children(auto&& fn) {}
    
  widget_id id() const { return uuid; }
  
  widget_id uuid;
  point sz = {0, 0}, pos = {0, 0};
};

template <class T>
concept is_child_event_listener = requires (T obj, mouse_event e, widget_ref r, 
                                            event_context& ec)
{
  obj.on_child_event(e, r, ec);
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
    ptr<void(widget_base*, point sz)> layout;
    ptr<widget_size_info(const widget_base*)> size_info;
    ptr<void(widget_base*, input_event, event_context&)> on;
    ptr<void(widget_base*, input_event, event_context&, widget_ref)> on_child_event;
    ptr<optional<widget_ref>(widget_base*, point)> find_child_at;
    ptr<void(widget_base*, widget_children_vec&)> children;
    ptr<void(widget_base*, int indent)> debug_dump;
    ptr<void(widget_base*, destroy_context ctx)> destroy;
    ptr<void(widget_base*, widget_tree&, widget_id)> mount;
    ptr<void(widget_base*, widget_tree&)> unmount;
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
  
  auto layout(point sz) {
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
  
  void mount(widget_tree& tree, widget_id parent) {
    vptr->mount(data, tree, parent);
  }
  
  void unmount(widget_tree& tree) {
    vptr->unmount(data, tree);
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
  
  point size() const { return data->size(); }
  
  rectangle area() const { return data->area(); }
  
  void set_size(point sz) { data->set_size(sz); }
  
  point position() const { return data->position(); }
  
  void set_position(point p) { data->set_position(p.x, p.y); }
  
  point absolute_position(const widget_tree& tree) const {
    return data->absolute_position(tree);
  }
  
  bool contains(point pos) const { return data->contains(pos); }
  
  auto find_child_at(point pos) const {
    return vptr->find_child_at(data, pos);
  }
  
  widget_id id() const { return data->id(); }
  
  std::vector<widget_ref> children() const {
    std::vector<widget_ref> vec; 
    vptr->children(data, vec); 
    return vec;
  }
  
  void debug_dump(int indent = 0) const {
    vptr->debug_dump(data, indent);
  }
  
  widget_base* raw_pointer() const { return data; }
};

struct widget_box : widget_ref {
  
  widget_box(std::nullptr_t) : widget_ref{nullptr, nullptr} {}
  
  template <class W>
    requires (std::is_base_of_v<widget_base, std::remove_reference_t<W>>)
  widget_box(W&& widget) {
    data = new std::decay_t<W> { WEAVE_FWD(widget) };
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
  
  widget_ref borrow() { return widget_ref{data, vptr}; }
  
  // FIXME: would be good to have a const_widget_ref?
  widget_ref borrow() const { return widget_ref{data, vptr}; }
  
  void reset(destroy_context ctx) {
    if (data) {
      vptr->destroy(data, ctx);
      data = nullptr;
    }
  }
  
  void destroy(destroy_context ctx) {
    reset(ctx);
  }
};

struct widget_tree {
  
  widget_id new_id() {
    while(tree.contains(widget_id{id_count}))
      ++id_count;
    return widget_id{id_count++};
  }
  
  optional<widget_ref> get(widget_id id) const {
    auto it = tree.find(id);
    if (it == tree.end())
      return {};
    return it->second.ref;
  }
  
  void insert(widget_ref ref, widget_id parent) {
    tree.insert_or_assign(ref.id(), node{parent, ref});
  }
  
  template <class W>
  void insert(W& w, widget_id parent) {
    insert(widget_ref(&w), parent);
  }
  
  void erase(widget_ref ref) {
    erase(ref.id());
  }
  
  void erase(widget_id id) {
    auto it = tree.find(id);
    assert( it != tree.end() && "widget already erased from tree" );
    tree.erase(it);
  }
  
  void erase(const widget_base& w) {
    erase(w.id());
  }
  
  template <class W>
  void erase_children(W& w) {
    w.traverse_children([this] (auto&& c) {
      erase(c.id());
      return true;
    });
  }
  
  template <class W>
  void insert_children(W& w) {
    w.traverse_children([this, &w] (auto&& c) {
      if constexpr (std::is_same_v<std::decay_t<decltype(c)>, widget_box>)
        insert(c.borrow(), w.id());
      else
        insert(widget_ref(&c), w.id());
      return true;
    });
  }
  
  void relocate(widget_ref ref) {
    auto it = tree.find(ref.id());
    assert( it != tree.end() && "called relocate on an element not in the tree" );
    it->second.ref = ref;
  }
  
  template <class W>
  void relocate(W& w) {
    relocate(widget_ref(&w));
  }
  
  struct parent_end_iterator {};
    
  struct parent_iterator {
    
    auto& operator++() {
      auto next = self.parent_of(id);
      if (next == id)
        done = true;
      else
        id = next;
      return *this;
    }
    
    widget_ref operator*() { 
      auto res = self.get(id);
      assert( res && "parent in parent range not in widget tree?" );
      return *res;
    }
    
    bool operator==(parent_end_iterator) const {
      return done;
    }
    
    bool operator!=(parent_end_iterator o) const {
      return !((*this) == o);
    }
    
    const widget_tree& self;
    widget_id id;
    bool done = false;
  };
  
  
  struct parent_range {
    
    auto begin() const { return parent_iterator{self, id, done}; }
    
    auto end() const { return parent_end_iterator{}; }
    
    const widget_tree& self;
    widget_id id;
    bool done = false;
  };
  
  widget_ref last_parent_of(widget_id id) const {
    while(true) {
      auto next = parent_ref(id);
      if (next.id() == id)
        return next;
      id = next.id();
    }
  }
  
  auto parents_of(widget_id id) const {
    auto p = parent_of(id);
    return parent_range{*this, p, p == id};
  }
  
  auto parents_of(widget_ref r) const {
    return parents_of(r.id());
  }
  
  auto parents_of(const widget_base& w) const {
    return parents_of(w.id());
  }
  
  widget_id parent_of(widget_id id) const {
    auto it = tree.find(id);
    assert( it != tree.end() && "widget not found in tree" );
    return it->second.parent;
  }
  
  widget_ref parent_ref(widget_id id) const {
    return *get(parent_of(id));
  }
  
  private : 
  
  struct node {
    widget_id parent;
    widget_ref ref;
  };
  
  std::unordered_map<widget_id, node> tree;
  unsigned id_count = 0;
};

void widget_base::destroy(this auto&& self, destroy_context ctx) {
  self.traverse_children([ctx] (auto&& c) {
    ctx.tree().erase(c.id());
    c.destroy(ctx);
    return true;
  });
}

point widget_base::absolute_position(const widget_tree& tree) const {
  auto p = position();
  for (auto parent : tree.parents_of(*this))
    p += parent.position();
  return p;
}

void widget_base::mount(this auto& self, widget_tree& tree, widget_id parent) {
  tree.insert(widget_ref(&self), parent);
  self.traverse_children( [&tree, pid = self.id()] (auto& child) {
    child.mount(tree, pid);
    return true;
  });
}

void widget_base::unmount(this auto& self, widget_tree& tree) {
  self.traverse_children( [&tree] (auto& child) {
    child.unmount(tree);
    return true;
  });
  tree.erase(self.id());
}

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

optional<widget_ref> widget_base::find_child_at(this auto& self, point pos) {
  optional<widget_ref> res;
  self.traverse_children( [&res, pos] (auto& elem) {
    return !elem.contains(pos) || (res = impl::to_widget_ref(elem), false);
  });
  return res;
}

struct event_result {
  
  event_result operator || (event_result er) const {
    return {rebuild_requested || er.rebuild_requested, 
            repaint_requested || er.repaint_requested};
  }
  
  bool rebuild_requested = false;
  bool repaint_requested = false;
};

struct graphics_context;

struct event_context {

  /// Returns a thread safe callable that signal that a rebuild is needed
  auto lift_rebuild_request();
  
  void request_rebuild() { frame_result.rebuild_requested = true; }
  void request_repaint() { frame_result.repaint_requested = true; }
  
  widget_ref parent_of(widget_base& b) const;
  
  void push_overlay(widget_box widget);
  
  void push_overlay_relative(widget_box widget);
  
  void pop_overlay(widget_id id);
  
  /// Register an animation fn to be executed every period_in_ms
  template <class W, class Fn>
  void animate(W& widget, Fn fn, int period_in_ms);
  
  /// Remove all animations for a widget
  void deanimate(widget_ref w);
  
  void grab_keyboard_focus(widget_id id);
  
  void release_keyboard_focus();
  
  widget_ref current_mouse_focus();
  
  void grab_mouse_focus(widget_id w);
  
  void reset_mouse_focus();
  
  auto& context() const { return ctx; }
  
  graphics_context& graphics_context() const; 
  
  template <class S>
  S& state() const { return *static_cast<S*>(state_ptr); }
  
  bool is_held(key_modifier mod) const;
  
  widget_tree& tree() const;
  
  application_context& ctx;
  event_result& frame_result;
  void* state_ptr;
};

namespace impl {
  
  template <class W>
  consteval auto child_event_fn_ptr() {
    if constexpr ( is_child_event_listener<W> )
      return + [] (widget_base* self, input_event e, event_context& ec, widget_ref child) {
        auto& Obj = *static_cast<W*>(self);
        if (e.index() == 0)
          Obj.on_child_event(get<0>(e), child, ec);
        else if constexpr ( requires { Obj.on_child_event(get<1>(e), child, ec); } ) {
          if (e.index() == 1)
            Obj.on_child_event(get<1>(e), child, ec);
        }
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
      +[] (widget_base* self, point sz) {
        auto& obj = *static_cast<W*>(self);
        obj.do_layout(sz);
      },
      +[] (const widget_base* self) -> widget_size_info {
        return static_cast<const W*>(self)->size_info();
      },
      +[] (widget_base* self, input_event e, event_context& ctx) {
        auto& Obj = *static_cast<W*>(self);
        if (e.index() == 0) {
          return Obj.on(get<0>(e), ctx);
        }
        else if (e.index() == 1) {
          // reacting to keyboard event is optional
          if constexpr ( requires { Obj.on(get<1>(e), ctx); } )
            return Obj.on(get<1>(e), ctx);
        }
        else
          return Obj.on_keyboard_focus_release(ctx);
      },
      child_event_fn_ptr<W>(),
      +[] (widget_base* self, point pos) -> optional<widget_ref> {
        auto& obj = *static_cast<W*>(self);
        return obj.find_child_at(pos);
      },
      +[] (widget_base* self, widget_children_vec& vec) {
        static_cast<W*>(self)->traverse_children( [&vec] (auto&& elem) {
          vec.push_back(to_widget_ref(elem));
          return true;
        });
      },
      +[] (widget_base* self, int indent) {
        static_cast<W*>(self)->debug_dump(indent + 1);
      },
      +[] (widget_base* self, destroy_context ctx) {
        static_cast<W*>(self)->destroy(ctx);
        delete static_cast<W*>(self);
      },
      +[] (widget_base* self, widget_tree& tree, widget_id id) {
        static_cast<W*>(self)->mount(tree, id);
      },
      +
      +[] (widget_base* self, widget_tree& tree) {
        static_cast<W*>(self)->unmount(tree);
      }
    };
  };
}

} // weave