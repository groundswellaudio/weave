#pragma once

#include "views_core.hpp"

#include <functional>
#include <vector>
#include <string>
#include <initializer_list>

namespace widgets {

struct list : widget_base {
  
  void on(mouse_event e, event_context_t<void>& ec) {
    if (!e.is_mouse_down())
      return;
    auto s = e.position.y / 13.f;
    if (s > cells.size()) {
      cell_selected = -1;
      return;
    }
    if (on_select_cell)
    {
      cell_selected = s;
      on_select_cell(ec, s);
    }
  }
  
  void update_size(graphics_context& ctx) {
    auto w = max_text_width(cells, ctx, 11) + 10;
    set_size({w, (float)cells.size() * 13.f});
  }
  
  void paint(painter& p) {
    float y = 5;
    vec2f pos{5, y};
    p.fill_style(colors::white);
    p.text_align(text_align::x::left, text_align::y::center);
    for (auto& c : cells) {
      p.text({pos.x, pos.y + 13 / 2}, c);
      pos.y += 13;
    }
    
    if (cell_selected != -1) {
      p.fill_style(button_overlay_color);
      p.rectangle({0, (float) cell_selected * 13}, {size().x, 13});
    }
  }
  
  std::vector<std::string> cells;
  std::function<void(event_context_t<void>& ec, int)> on_select_cell;
  int cell_selected = -1;
};

} // widgets

namespace views {

template <class T>
struct list : view<list<T>> {
  
  template <class R>
  list(R&& data, bool RebuildWhen) : data{(R&&)data}, rebuild_when{RebuildWhen} {}
  
  template <class E>
  list(std::initializer_list<E> init, bool RebuildWhen) : data{init}, rebuild_when{RebuildWhen} {}
  
  using widget_t = widgets::list;
  
  void set_cells(widget_t& w) {
    w.cells.clear();
    for (auto&& e : data)
      w.cells.push_back(std::string(e));
  }
  
  auto build(const widget_builder& b, ignore) {
    widget_t res;
    set_cells(res);
    res.update_size(b.context().graphics_context());
    res.on_select_cell = select_cell;
    return res;
  }
  
  rebuild_result rebuild(ignore old, widget_ref w, ignore, ignore) {
    if (rebuild_when) {
      auto& wb = w.as<widget_t>();
      set_cells(wb);
    }
    return {};
  }
  
  template <class S, class RT, class... Args>
  auto& on_select_cell(member_fn_ptr<RT, S, Args...> fn) {
    select_cell = [fn] (event_context_t<void>& ec, int cell) {
      std::invoke(fn, *static_cast<S*>(ec.state()), cell);
    };
    return *this;
  }
  
  template <class S, class RT, class... Args>
  auto& on_file_drop(member_fn_ptr<RT, S, Args...> fn) {
    file_drop_fn = [fn] (event_context_t<void>& ec, const std::string& str) {
      std::invoke(fn, *static_cast<S*>(ec.state()), str);
    };
    return *this;
  }
  
  T data;
  std::function<void(event_context_t<void>& ec, int)> select_cell;
  std::function<void(event_context_t<void>& ec, const std::string&)> file_drop_fn;
  bool rebuild_when = false;
};

template <class T>
list(std::initializer_list<T>, bool) -> list<std::vector<T>>;

template <class R>
list(R&&, bool) -> list<R>;

template <class R>
list(R&, bool) -> list<const R&>;

} // views