#pragma once

#include "views_core.hpp"
#include "modifiers.hpp"
#include <functional>
#include <concepts>

namespace weave::widgets {

template <class W>
struct selectable : W {
  
  using base_t = W;
  
  auto& base() { return (W&)*this; }
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_down() && !is_selected)
    {
      is_selected = true;
      on_select();
      ec.request_repaint();
      ec.request_rebuild();
    }
    W::on(e, ec);
  }
  
  void paint(painter& p) {
    W::paint(p);
    if (is_selected) {
      p.fill_style(rgba_f{colors::white}.with_alpha(0.3));
      p.fill(rounded_rectangle(this->size()));
    }
  }
  
  bool is_selected = false;
  std::function<void()> on_select; 
};

struct multi_selection_actions {

  struct vtable {
    template <class T>
    using ptr = T*; 
    ptr<void(void*)> clear;
    ptr<void(void*, unsigned)> add;
    ptr<void(void*, unsigned)> select_range;
  };
  
  template <class T>
  static constexpr vtable vtable_for = {
    +[] (void* data) { 
      auto& obj = *static_cast<T*>(data);
      obj.clear(); 
    },
    +[] (void* data, unsigned id) { 
      auto& obj = *static_cast<T*>(data);
      obj.insert(id); 
    },
    +[] (void* data, unsigned id) {
      auto& obj = *static_cast<T*>(data);
      if (obj.empty()) {
        obj.insert(id);
        return;
      }
      auto min = *obj.begin();
      auto max = *(--obj.end());
      auto set_range = [&] (unsigned a, unsigned b) {
        obj.clear();
        obj.insert_range(iota(a, b));
      };
      
      if (id < min) 
        set_range(id, min + 1);
      else 
        set_range(min, id + 1);
    }
  };
  
  // T must be an ordered container of integer like set<unsigned> or flat_set
  template <class T>
  void set(T* ptr) {
    data = ptr;
    vtable_ptr = &vtable_for<T>;
  }
  
  void clear() { vtable_ptr->clear(data); }
  void add(unsigned id) { vtable_ptr->add(data, id); }
  void select_range(unsigned id) { vtable_ptr->select_range(data, id); }
  
  void* data = nullptr;
  const vtable* vtable_ptr; 
};

template <class W>
struct multi_selectable : W {
  
  using base_t = W;
  
  W& base() { return *this; }
  
  void on_mouse_event(mouse_event e, event_context& ec) {
    if (!e.is_down())
      return;
    if (ec.is_held(key_modifier::gui)) { // incremental selection
      selection_actions.add(selection_id);
    }
    else if (ec.is_held(key_modifier::shift)) {
      selection_actions.select_range(selection_id);
    }
    else {
      selection_actions.clear();
      selection_actions.add(selection_id);
    }
    is_selected = true;
    ec.request_rebuild();
    ec.request_repaint();
  }
  
  void on_child_event(mouse_event e, widget_ref, event_context& ec) {
    on_mouse_event(e, ec);
  }
  
  void on(mouse_event e, event_context& ec) {
    on_mouse_event(e, ec);
    W::on(e, ec);
  }
  
  void paint(painter& p) {
    W::paint(p);
    if (is_selected) {
      p.fill_style(rgba_f{colors::white}.with_alpha(0.3));
      p.fill(rounded_rectangle(W::size()));
    }
  }
  
  multi_selection_actions selection_actions;
  unsigned selection_id;
  bool is_selected = false;
};

} // widgets

namespace weave::views {

template <class V, class S, class Id>
struct selectable : view<selectable<V, S, Id>>, view_modifiers {
  
  using widget_t = widgets::selectable<typename V::widget_t>;
  
  selectable(V v, S* selection, Id id) 
    requires (std::assignable_from<S&, Id>)
  : view{WEAVE_MOVE(v)}, selection{selection}, id{id} {}
  
  auto build(const build_context& b, auto& state) {
    auto res = widget_t{ view.build(b, state) };
    res.is_selected = *selection == id;
    res.on_select = [ptr = selection, id = id] () { *ptr = id; };
    return res;
  }
  
  auto rebuild(auto& old, widget_ref r, const build_context& ctx, auto& state) {
    using widget_t = widgets::selectable<decltype(view.build(ctx, state))>;
    auto& ws = r.as<widget_t>();
    ws.is_selected = *selection == id;
    ws.on_select = [ptr = selection, id = id] () { *ptr = id; };
    return view.rebuild(old.view, widget_ref{&ws.base()}, ctx, state);
  }
  
  void destroy(widget_ref r, application_context& ctx) {
    view.destroy(widget_ref(&r.as<widget_t>().base()), ctx);
  }
  
  V view;
  S* selection;
  Id id;
};

template <class V, class S, class Id>
struct multi_selectable : view<multi_selectable<V, S, Id>>, view_modifiers {
  
  using widget_t = widgets::multi_selectable<typename V::widget_t>;
  
  multi_selectable(V view, S* selection, Id id) 
  : view{WEAVE_MOVE(view)}, selection{selection}, id{id}
  {}
  
  auto build(const build_context& ctx, auto& state) {
    auto res = widget_t{ view.build(ctx, state) };
    res.is_selected = selection->contains(id);
    res.selection_actions.set(selection);
    res.selection_id = id;
    return res;
  }
  
  auto rebuild(const multi_selectable<V, S, Id>& old, widget_ref r, 
               const build_context& ctx, auto& state) {
    auto& wb = r.as<widget_t>();
    wb.is_selected = selection->contains(id);
    wb.selection_actions.set(selection);
    wb.selection_id = id;
    return view.rebuild(old.view, widget_ref(&wb.base()), ctx, state);
  }
  
  V view;
  S* selection;
  Id id;
};

} // views