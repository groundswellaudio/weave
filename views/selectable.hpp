#pragma once

#include "views_core.hpp"
#include <functional>

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

} // widgets

namespace weave::views {

template <class V, class S, class Id>
struct selectable : view<selectable<V, S, Id>> {
  
  selectable(V v, S* selection, Id id) 
  : view{WEAVE_MOVE(v)}, selection{selection}, id{id} {}
  
  auto build(const build_context& b, auto& state) {
    using widget_t = widgets::selectable<decltype(view.build(b, state))>;
    auto res = widget_t{ view.build(b, state) };
    res.is_selected = *selection == id;
    res.on_select = [ptr = selection, id = id] () { *ptr = id; };
    return res;
  }
  
  auto rebuild(auto& old, widget_ref r, auto&& ctx, auto& state) {
    using widget_t = widgets::selectable<decltype(view.build(ctx, state))>;
    auto& ws = r.as<widget_t>();
    ws.is_selected = *selection == id;
    return view.rebuild(old.view, widget_ref{&ws.base()}, ctx, state);
  }
  
  V view;
  S* selection;
  Id id;
};

} // views