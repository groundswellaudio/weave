#pragma once

#include "spiral.hpp"

struct State : app_state {
  
  struct Table {
    
    static constexpr const char* properties_v[] = {
      "Name", "Artist", "Album"
    };
    
    auto&& properties () const {
      return properties_v;
    }
    
    unsigned num_properties() const { return 3; }
    
    struct cell {
      std::string properties[3];
    };
    
    std::vector<cell> cells;
  };
  
  State() 
  {
    table.cells.push_back({"ksian boktet", "jupiter", "gloom"});
    table.cells.push_back({"autechre", "bleep bloop", "scrunch"});
    table.cells.push_back({"alicia keys", "unthinkable", "really good"});
  }
  
  std::string PlaylistName;
  Table table;
};

struct table_widget : widget_base {
  
  using value_type = void; 
  using Self = table_widget;
  
  table_widget(vec2f sz) : widget_base{sz} {
    
  }
  
  struct cell {
    std::vector<std::string> prop;
    bool selected = false;
  };
  
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
    auto it = properties.begin();
    ++it;
    for (; it != properties.end(); ++it) {
      if (get<1>(*it) > pos.x)
        return vec2i{k, selected_row};
      ++k;
    }
    return vec2i{k, selected_row};
  }
  
  void handle_mouse_down_body(mouse_event e, event_context<Self>& Ec) {
    auto cell = find_cell_at(e.position);
    if (!cell)
      return;
    auto [col, row] = *cell;
    bool select_range = Ec.context().is_active(key_modifier::shift);
    if (select_range) {
      auto i = selection.x;
      selection.x = std::min(i, row);
      selection.y = std::max(i, row);
    }
    else {
      if (focused_cell == cell) {
      // Clicking on a single item again : edit the field
        auto lens = [this, cell] (ignore) -> auto& { return cells[cell->y].prop[cell->x]; };
        auto dl = dyn_lens<std::string>(make_lens(lens), tag<void*>{});
        auto field_width = (col == properties.size() - 1) 
          ? size().x - get<1>(properties[col])
          : get<1>(properties[col + 1]) - get<1>(properties[col]);
        vec2f field_size {field_width, row_height};
        edited_field.emplace( with_lens_t<text_field_widget>{{field_size}, std::move(dl)} );
        edited_field->set_position( {get<1>(properties[col]), first_row + row * row_height} );
        edited_field->editing = true;
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
  
  void on(mouse_event e, event_context<table_widget>& Ec) 
  {
    if (e.is_mouse_down()) {
      if (e.position.y < first_row) 
        handle_mouse_down_header(e.position);
      else 
        handle_mouse_down_body(e, Ec);
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
        p.text( {5 + properties[k++].m1, pos + row / 2}, prop );
      }
      pos += row;
    }
  }
  
  vec2<int> selection {-1, -1};
  std::vector<tuple<std::string, float>> properties;
  std::vector<cell> cells;
  std::optional<vec2i> focused_cell;
  std::optional<with_lens_t<text_field_widget>> edited_field;
  int dragging = -1;
  std::function<void(event_context_t<void>& ec, vec2i, const std::string&)> on_field_edit;
};

template <class T>
struct table_v : view<table_v<T>> {
  
  table_v(T& table) : table{table} {}
  
  auto build(auto&& builder, auto& state) {
    table_widget res {{400, 400}};
    res.set_properties(table.properties());
    for (auto&& e : table.cells) {
      res.cells.emplace_back();
      for (auto& p : e.properties)
        res.cells.back().prop.push_back(p);
    }
    return res;
  }
  
  rebuild_result rebuild(auto& old, widget_ref w, ignore, auto& state) {
    return {};
  }
  
  T& table;
};

auto make_view(State& state)
{
  return vstack{ table_v { state.table },
                 text_field{ &State::PlaylistName }
               }.align_center();
}

inline void run_app() {
  State state;
  auto app = make_app(state, &make_view);
  app.run(state);
}