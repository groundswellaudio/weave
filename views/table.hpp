#pragma once

#include "views_core.hpp"
#include "scrollable.hpp"

#include <functional>
#include <ranges>

namespace widgets {

struct table : widget_base, scrollable_base {
  
  static constexpr float first_row = 30;
  static constexpr float row_height = 15;
  
  struct selection_t : vec2i {
    void traverse(auto&& fn) const {
      if (x == -1)
        return;
      if (y == -1) {
        fn(x);
        return;
      }
      for (auto i : iota(x, y))
        fn(i);
    }
  };
  
  rectangle scroll_zone() const {
    return {{0, first_row}, {size().x, size().y - first_row}};
  }
  
  void displace_scroll(float delta) {
    scroll_offset += delta;
    scroll_offset = std::max(scroll_offset, 0.f);
    if (edited_field)
      edited_field->set_position(edited_field->position() - vec2f{0, delta});
    //assert( scroll_offset >= 0 );
  }
  
  float scroll_size() const {
    return cells.size() * row_height;
  }
  
  struct cell {
    std::vector<std::string> prop;
    bool selected = false;
  };
  
  selection_t selection {-1, -1};
  std::vector<tuple<std::string, float>> properties;
  std::vector<cell> cells;
  std::optional<vec2i> focused_cell;
  std::optional<text_field> edited_field;
  int dragging = -1;
  float scroll_offset = 0;
  widget_action<vec2i, std::string_view> on_field_edit;
  widget_action<int> cell_double_click;
  widget_action<const std::string&> on_file_drop;
  widget_action<popup_menu(selection_t)> popup;
  
  using Self = table;
  
  table(vec2f sz) : widget_base{sz} {}
  
  widget_size_info size_info() const {
    return {{properties.size() * 30.f, first_row + row_height * 3}};
  }
  
  vec2f expand_factor() const {
    return {1, 1};
  }
  
  void update_properties(auto&& range) {
    auto it = std::ranges::begin(range);
    for (auto& p : properties)
      p.m0 = *it++;
    auto pos_end = properties.back().m1;
    while (it != std::ranges::end(range))
    {
      pos_end += size().x / std::ranges::size(range);
      properties.push_back({*it++, pos_end});
    }
  }
  
  void set_properties(auto&& range) {
    float posx = 0;
    for (auto& e : range) {
      properties.push_back({std::string{e}, posx});
      posx += size().x / std::ranges::size(range);
    }
  }
  
  void handle_mouse_down_header(vec2f p) {
    for (unsigned k = 1; k < properties.size(); ++k) 
    {
      auto& [_, posx] = properties[k];
      float x = p.x;
      if (x < posx - 5)
        break;
      if (std::abs(x - posx) < 10)
      {
        dragging = k; 
        break;
      }
    }
  }
  
  std::optional<vec2i> find_cell_at(vec2f pos) const {
    int selected_row = (scroll_offset + pos.y - first_row) / row_height;
    if (selected_row >= cells.size())
      return {};
    int k = 0;
    auto it  = properties.begin();
    ++it;
    for (;it != properties.end(); ++it)
    {
      if (get<1>(*it) > pos.x)
        return vec2i{k, selected_row};
      ++k;
    }
    return vec2i{k, selected_row};
  }
  
  void handle_mouse_down_body(mouse_event e, event_context& Ec) {
    if (e.is_right_click() && popup) {
      auto w = popup(Ec, selection);
      w.set_position(e.position + position() + Ec.absolute_position());
      enter_popup_menu(Ec, std::move(w));
      return;
    }
    auto cell = find_cell_at(e.position);
    if (!cell)
      return;
    auto [col, row] = *cell;
    if (e.is_double_click() && cell_double_click) {
      cell_double_click(Ec, row);
      return;
    }
    bool select_range = Ec.context().is_active(key_modifier::shift);
    if (select_range) {
      auto i = selection.x;
      selection.x = std::min(i, row);
      selection.y = std::max(i, row);
    }
    else {
      if (focused_cell == cell) {
      // Clicking on a single item again : edit the field
        auto field_width = (col == properties.size() - 1) 
          ? size().x - get<1>(properties[col])
          : get<1>(properties[col + 1]) - get<1>(properties[col]);
        vec2f field_size {field_width, row_height};
        edited_field.emplace( text_field{{field_size}} );
        edited_field->set_position( {get<1>(properties[col]), first_row + row * row_height - scroll_offset} );
        edited_field->editing = true;
        edited_field->value_str = cells[cell->y].prop[cell->x];
        edited_field->write = [this, cell] (event_context& Ec, auto&& str) { 
          cells[cell->y].prop[cell->x] = str;
          if (on_field_edit)
            on_field_edit(Ec, *cell, str);
          edited_field.reset(); 
        };
        Ec.with_parent(this).grab_mouse_focus(&*edited_field);
        Ec.with_parent(this).grab_keyboard_focus(&*edited_field);
      }
      else {
        selection.x = row;
        selection.y = -1;
      }
      focused_cell = *cell;
    }
  }
  
  void on(mouse_event e, event_context& Ec) 
  {
    if (e.is_mouse_down() && edited_field) {
      edited_field.reset();
      Ec.request_repaint();
    }
    
    if (scrollable_base::on(e, Ec))
      return;
    
    if (e.is_file_drop() && on_file_drop) {
      on_file_drop(Ec, e.dropped_file());
      return;
    }
    
    if (e.is_mouse_down()) {
      if (e.position.y < first_row) 
        handle_mouse_down_header(e.position);
      else 
        handle_mouse_down_body(e, Ec);
      Ec.request_repaint();
      return;
    }
    
    if (e.is_mouse_drag() && dragging != -1) {
      constexpr float margin = 50;
      int k = dragging;
      auto& posx = properties[k].m1;
      posx = e.position.x;
      float min = properties[k-1].m1 + margin;
      auto col_end = k == properties.size() - 1 ? size().x : properties[k + 1].m1;
      if (posx < min)
        posx = min;
      else {
        auto max = col_end - margin;
        posx = std::min(max, posx);
      }
      if (edited_field) { 
        auto p = edited_field->position();
        if (focused_cell->x == dragging) {
          edited_field->set_position({posx, p.y});
          auto w = col_end - properties[dragging].m1;
          edited_field->set_size({w, edited_field->size().y});
        }
        else if (focused_cell->x + 1 == dragging) {
          auto w = properties[dragging].m1 - properties[focused_cell->x].m1; 
          edited_field->set_size( {w, edited_field->size().y} );
        }
      }
      Ec.request_repaint();
      return;
    }
    
    if (e.is_mouse_up())
      dragging = -1;
  }
  
  bool traverse_children(auto fn) {
    return edited_field ? fn(*edited_field) : true;
  }
  
  void paint_selection_overlay(painter& p) {
    if (selection.x != -1) {
      p.fill_style(rgba{colors::cyan}.with_alpha(70));
      auto pos_y = selection.x * row + first_row;
      if (selection.y != -1)
        p.rectangle( {0, pos_y}, {size().x, (selection.y - selection.x + 1) * row} );
      else
        p.rectangle( {0, pos_y}, {size().x, row} );
    }
  }
  
  static constexpr auto row = 15.f;
  static constexpr float margin = 5;
  
  void paint_body(painter& p, vec2f body_sz) {
    p.fill_style(colors::white);
    int cells_begin = scroll_offset / row;
    int cells_end = (scroll_offset + scroll_zone().size.y) / row;
    cells_end = std::min(cells_end, (int) cells.size());
    
    float pos = (int) -scroll_offset % (int) row;
    
    assert( cells_begin >= 0 );
    for (int i = cells_begin; i < cells_end; ++i, pos += row) {
      auto& c = cells[i];
      int k = 0;
      for (auto& prop : c.prop) {
        float width = k == c.prop.size() - 1 
          ? (size().x - properties[k].m1) 
          : properties[k+1].m1 - properties[k].m1;
        
        p.text_bounded({margin + properties[k++].m1, pos + row / 2}, width - margin * 2, prop);
      }
    }
    
    if (selection.x != -1 && selection.x >= cells_begin && selection.x < cells_end) {
      p.fill_style(rgba{colors::cyan}.with_alpha(70));
      auto pos_y = selection.x * row - scroll_offset;
      if (selection.y != -1)
        p.rectangle( {0, pos_y}, {size().x, (selection.y - selection.x + 1) * row} );
      else
        p.rectangle( {0, pos_y}, {size().x, row} );
    }
  }
  
  void paint(painter& p) {
    // background
    auto background_col = rgb_f(colors::gray) * 0.3;
    
    p.fill_style(rgba_f(colors::black) * 0.3);
    p.rectangle({0, 0}, {size().x, first_row});
    p.font_size(13);
    p.fill_style(colors::white);
    float pos_x = 0;
    p.stroke_style(colors::black);
    //p.line( {0, first_row}, {size().x, first_row}, 1 );
    
    p.text_align(text_align::x::left, text_align::y::center);
    for (auto& [prop, posx] : properties) {
      p.line( {posx, 0}, {posx, first_row}, 1 );
      p.text( {5 + posx, first_row / 2}, prop );
    }
    
    p.stroke_style(rgba_f(colors::black) * 0.5);
    p.stroke_rect({0, 0}, size(), 1);
    
    {
      auto _ = p.translate({0, first_row});
      p.scissor({0, 0}, scroll_zone().size);
      paint_body(p, size() - vec2f{0, first_row});
      p.reset_scissor();
    }
    
    scrollable_base::paint(p);
  }
};

} // widgets

/// The trait used to determine how to present a type
template <class T>
struct table_model;

namespace views {

template <class T>
  requires complete_type<table_model<T>>
struct table : view<table<T>> {
  
  table(T data, bool RebuildWhen) : data{data}, rebuild_when{RebuildWhen} {}
  
  using widget_t = widgets::table;
  using model_t = table_model<std::decay_t<T>>;
  
  void set_cells(widget_t& w) {
    for (auto&& e : model_t{}.cells(data)) {
      w.cells.emplace_back();
      for (auto& p : model_t{}.cell_properties(e))
        w.cells.back().prop.push_back(p);
    }
  }
  
  auto build(auto&& builder, auto& state) {
    widget_t res {{400, 400}};
    res.set_properties(model_t{}.properties(data));
    set_cells(res);
    res.cell_double_click = cell_double_click;
    res.on_file_drop = file_drop_fn;
    res.popup = popup_opener;
    return res;
  }
  
  rebuild_result rebuild(auto& old, widget_ref w, ignore, auto& state) {
    if (rebuild_when) {
      auto& wb = w.as<widget_t>();
      wb.update_properties(model_t{}.properties(data));
      wb.cells.clear();
      set_cells(wb);
    }
    return {};
  }
  
  template <class S, class RT, class... Args>
  auto& on_cell_double_click(member_fn_ptr<RT, S, Args...> fn) {
    cell_double_click = [fn] (event_context& ec, int cell) {
      context_invoke<S>(fn, ec, cell);
    };
    return *this;
  }
  
  template <class S, class M>
  auto& on_cell_double_click(auto fn) {
    cell_double_click = [fn] (event_context& ec, int cell) {
      context_invoke<S>(fn, ec, cell);
    };
    return *this;
  }
  
  template <class S, class RT, class... Args>
  auto& on_file_drop(member_fn_ptr<RT, S, Args...> fn) {
    file_drop_fn = [fn] (event_context& ec, auto&& file) {
      context_invoke<S>(fn, ec, file);
    };
    return *this;
  }
  
  auto& on_file_drop(auto fn) {
    file_drop_fn = [fn] (event_context& ec, auto&& file) {
      ec.request_rebuild();
      fn(ec, file);
    };
    return *this;
  }
  
  auto& popup_menu(auto fn) {
    popup_opener = fn;
    return *this;
  }
  
  T data;
  widget_action<int> cell_double_click;
  widget_action<const std::string&> file_drop_fn;
  widget_action<widgets::popup_menu(widgets::table::selection_t)> popup_opener;
  bool rebuild_when = false;
};

template <class T>
table(T&) -> table<T&>;

template <class T>
table(T&&) -> table<T>;

} // views