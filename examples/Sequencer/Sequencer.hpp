#pragma once

#include "spiral.hpp"

struct State {

};

struct sequencer : widget_base {
  
  struct Note : widget_base {
    
    void on(mouse_event e, event_context& ec) {
      if (e.is_drag()) {
        e.drag_start()
      }
    }
    
  };
  
  void on(mouse_event e, event_context& ec) {
    
  }
  
  auto traverse_children(auto&& fn) {
    return std::all_of(notes.begin(), notes.end(), fn);
  }
  
  std::vector<Note> notes;
  std::optional<rectangle> lasso;
};

struct sequencer_view {
  
  using view_state = selection_group<int>;
  
  auto body(State& s, view_state& selection) {
    using namespace views;
    return zstack { 
        for_each( state.notes, [idx = 0] (auto N) mutable {
          return views::rectangle{N, {N.length, 10}}.on_double_click( [p = idx++] (State& s) { s.remove_note(p); } )
                                                    .selectable( selection.setter(idx) );
        })
    }.on_double_click( [] (State& s, vec2f pos) { 
      s.add_note(pos.x, pos.y / 15);
    }).with_selection_lasso()
      .on_key_down( keycode::erase, [p = &selection] (State& ec) {
        s.remove_notes(*p);
      })
      .on_key_down( keycode::arrow_left )
      .on_child_event( [p = &state, first_pos = vec2f{0, 0}] (mouse_event e, event_context& ec, widget_ref child) {
        if (e.is_mouse_down())
          first_pos = (int) e.position.x / p->grid_scale;
        if (e.is_mouse_drag()) {
          auto new_pos = (int) e.position.x / p->grid_scale;
          if (new_pos != first_pos) {
            ec.state<State>().move_notes(p->selection, new_pos - first_pos);
            first_pos = new_pos;
          }
        }
      }
  }
  
};

auto make_view(State& state) {
  using namespace views;
}

inline void run_app() {
  window_properties prop;
  prop.name = "TextureSynthesis";
  auto app = make_app(state, &make_view, prop);
  app.run(state);
}