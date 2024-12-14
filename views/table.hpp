#pragma once

#include "views_core.hpp"

#include <functional>
#include <ranges>

namespace widgets {

struct table : widget_base {
  
  using Self = table;
  
  table(vec2f sz) : widget_base{sz} {}
  
  struct cell {
    std::vector<std::string> prop;
    bool selected = false;
  };
  
  void update_properties(auto&& range) {
    auto it = std::ranges::begin(range);
    for (auto& p : properties)
      p.m0 = *it++;
    auto pos_end = properties.back().m1;
    while (it != std::ranges::end(range))
    {
      pos_end += 50;
      properties.push_back({*it++, pos_end});
    }
  }
  
  void set_properties(auto&& range) {
    float posx = 0;
    for (auto& e : range) {
      properties.push_back({std::string{e}, posx});
      posx += 50;
    }
  }
  
  static constexpr float first_row = 22;
  static constexpr float row_height = 15;
  
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
    int selected_row = (pos.y - first_row) / row_height;
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
        edited_field->set_position( {get<1>(properties[col]), first_row + row * row_height} );
        edited_field->editing = true;
        edited_field->value_str = cells[cell->y].prop[cell->x];
        edited_field->write = [this, cell] (event_context& Ec, auto&& str) { 
          cells[cell->y].prop[cell->x] = str;
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
    if (e.is_file_drop() && on_file_drop) {
      on_file_drop(Ec, e.dropped_file());
      return;
    }
    
    if (e.is_mouse_down()) {
      if (e.position.y < first_row) 
        handle_mouse_down_header(e.position);
      else 
        handle_mouse_down_body(e, Ec);
      return;
    }
    
    if (e.is_mouse_drag() && dragging != -1) {
      constexpr float margin = 50;
      int k = dragging;
      auto& posx = properties[k].m1;
      posx = e.position.x;
      float min = properties[k-1].m1 + margin;
      if (posx < min)
        posx = min;
      else {
        auto max = (k == properties.size() - 1 ? size().x : properties[k + 1].m1)
                  - margin;
        posx = std::min(max, posx);
      }
      return;
    }
    
    if (e.is_mouse_up())
      dragging = -1;
  }
  
  bool traverse_children(auto fn) {
    return edited_field ? fn(*edited_field) : true;
  }
  
  void paint(painter& p) {
    constexpr float first_row = 22;
    constexpr auto row = 15.f;
    p.font_size(13);
    p.fill_style(colors::white);
    p.text_align(text_align::x::left, text_align::y::center);
    float pos_x = 0;
    p.stroke_style(colors::white);
    p.line( {0, first_row}, {size().x, first_row}, 1 );
    
    for (auto& [prop, posx] : properties) {
      p.line( {posx, 0}, {posx, size().y}, 1 );
      p.text( {5 + posx, first_row / 2}, prop );
    }
    p.stroke_rect({0, 0}, size(), 1);
    
    // selection overlay
    if (selection.x != -1) {
      p.fill_style(rgba{colors::cyan}.with_alpha(70));
      auto pos_y = selection.x * row + first_row;
      if (selection.y != -1)
        p.rectangle( {0, pos_y}, {size().x, (selection.y - selection.x + 1) * row} );
      else
        p.rectangle( {0, pos_y}, {size().x, row} );
    }
    
    float pos = first_row;
    p.fill_style(colors::white);
    for (auto& c : cells) {
      int k = 0;
      
      for (auto& prop : c.prop) {
        float width = k == c.prop.size() - 1 
          ? (size().x - properties[k].m1) 
          : properties[k+1].m1 - properties[k].m1;
        
        p.text_bounded( {5 + properties[k++].m1, pos + row / 2}, width - 5 * 2, prop );
      }
      pos += row;
    }
  }
  
  vec2i selection {-1, -1};
  std::vector<tuple<std::string, float>> properties;
  std::vector<cell> cells;
  std::optional<vec2i> focused_cell;
  std::optional<text_field> edited_field;
  int dragging = -1;
  widget_action<vec2i, std::string_view> on_field_edit;
  widget_action<int> cell_double_click;
  widget_action<const std::string&> on_file_drop;
};

} // widgets

/// The trait used to determine how to present a type
template <class T>
struct table_model;

namespace views {

template <class T>
  requires complete_type<table_model<T>>
struct table : view<table<T>> {
  
  table(T& data, bool RebuildWhen) : data{data}, rebuild_when{RebuildWhen} {}
  
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
      std::invoke(fn, ec.state<S>(), cell);
    };
    return *this;
  }
  
  template <class S, class M>
  auto& on_cell_double_click(auto fn) {
    cell_double_click = [fn] (event_context& ec, int cell) {
      std::invoke(fn, ec.state<S>());
    };
    return *this;
  }
  
  template <class S, class RT, class... Args>
  auto& on_file_drop(member_fn_ptr<RT, S, Args...> fn) {
    file_drop_fn = [fn] (event_context& ec, auto&& file) {
      std::invoke(fn, ec.state<S>(), file);
    };
    return *this;
  }
  
  auto& on_file_drop(auto fn) {
    file_drop_fn = [fn] (event_context& ec, auto&& file) {
      fn(ec, file);
    };
    return *this;
  }
  
  T& data;
  widget_action<int> cell_double_click;
  widget_action<const std::string&> file_drop_fn;
  bool rebuild_when = false;
};

} // views