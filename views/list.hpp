#pragma once

#include "views_core.hpp"

#include <functional>
#include <vector>
#include <string>
#include <initializer_list>

namespace weave::widgets {

struct list : widget_base {
  
  static constexpr float margin = 5.f;
  static constexpr float row_size = 13.f;
  
  void on(mouse_event e, event_context& ec) {
    if (!e.is_down())
      return;
    auto s = (e.position.y - margin) / row_size;
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
  
  vec2f min_size() const { return {50, row_size * 3}; }
  vec2f max_size() const { return {infinity<float>(), infinity<float>()}; }
  
  vec2f expand_factor() const { return {1, 1}; }
  void update_size(graphics_context& ctx) {
    auto w = max_text_width(cells, ctx, 11) + 2 * margin;
    w = std::max(w, 30.f);
    auto h = (float)cells.size() * row_size;
    h = std::max(h, 30.f);
    set_size({w, (float)cells.size() * row_size});
  }
  
  void paint(painter& p) {
    float y = margin;
    vec2f pos{margin, y};
    p.fill_style(colors::white);
    p.text_align(text_align::x::left, text_align::y::center);
    for (auto& c : cells) {
      p.text({pos.x, pos.y + row_size / 2}, c);
      pos.y += row_size;
    }
    
    if (cell_selected != -1) {
      p.fill_style(button_overlay_color);
      p.fill( rectangle({0, (float) cell_selected * row_size + margin}, {size().x, row_size}) );
    }
  }
  
  std::vector<std::string> cells;
  widget_action<int> on_select_cell;
  int cell_selected = -1;
};

} // widgets

namespace weave::views {

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
  
  auto build(const build_context& b, ignore) {
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
    select_cell = [fn] (event_context& ec, int cell) {
      std::invoke(fn, ec.state<S>(), cell);
    };
    return *this;
  }
  
  template <class S, class RT, class... Args>
  auto& on_file_drop(member_fn_ptr<RT, S, Args...> fn) {
    file_drop_fn = [fn] (event_context& ec, const std::string& str) {
      std::invoke(fn, ec.state<S>(), str);
    };
    return *this;
  }
  
  T data;
  widget_action<int> select_cell;
  widget_action<const std::string&> file_drop_fn;
  bool rebuild_when = false;
};

template <class T>
list(std::initializer_list<T>, bool) -> list<std::vector<T>>;

template <class R>
list(R&&, bool) -> list<R>;

template <class R>
list(R&, bool) -> list<const R&>;

} // views