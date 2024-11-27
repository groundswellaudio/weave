#pragma once

#include "spiral.hpp"

#include <ranges>
#include <random>
#include <future>

struct State {
  
  struct Table {
    
    unsigned num_properties() const { return 4; }
    
    std::string name, artist, album;
    int date_added;
  };
  
};

struct table_widget {
  
  struct cell {
    std::vector<std::string> prop; 
  };
  
  void on(mouse_event e, event_context<table_widget>& Ec) {
    if (e.is_right_click()) {
      
    }
  }
  
  void paint(painter& p) {
    auto row = 15;
    p.fill_style(colors::white);
    for (auto& c : cells) {
      p.text_align( text_alignment::x::left, text_alignment::y::center );
      p.text( {5, pos + row / 2}, c.prop );
      pos += row;
    }
  }
  
  std::vector<cell> cells;
};

template <class T>
struct table_v {
  
  auto build(auto&& builder, auto& state) {
    for (auto p : table.properties())
    for (auto&& e : table.cells()) {
      for (auto p : e.properties())
    }
  }
  
  T table;
};

auto make_view(TextureSynthesis& state)
{
  return table { state.table };
}

inline void run_texture_synth() {
  TextureSynthesis state;
  auto app = make_app(state, &make_texture_synth);
  app.run(state);
}