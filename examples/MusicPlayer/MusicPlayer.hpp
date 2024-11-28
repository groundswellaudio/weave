#pragma once

#include "spiral.hpp"

#include <ranges>
#include <random>
#include <future>

struct State {
  
  struct Table {
    
    static constexpr const char* props[] {"Track", "Artist", "Album"};
    
    auto&& properties() const { return props; }
    
    struct Cell {
      auto& properties() const { return data; }
      
      std::string data[3];
    };
    
    Table() {
      cells.push_back({"ksianbok", "gaping anus", "where is everyone"});
      cells.push_back({"autechre", "are you ready (hardcore mix 2012)", "quecha jackets"});
      cells.push_back({"tuffstuff", "king of idiots", "clownerie finale"});
    }
    
    std::vector<Cell> cells;
  };
  
  Table table;
};

struct table_widget : widget_base {
  
  using value_type = void;
  
  struct cell {
    std::vector<std::string> prop; 
  };
  
  void on(mouse_event e, event_context<table_widget>& Ec) {
    if (e.is_right_click()) {
      
    }
  }
  
  void paint(painter& p) {
    float row = 15;
    p.fill_style(colors::white);
    p.text_align( text_align::x::left, text_align::y::center );
    p.font_size(13.f);
    
    float col = 100;
    float posx = 5;
    for (auto& prop : properties) {
      p.text({posx, row / 2}, prop);
      posx += col;
    }
    
    float posy = row;
    for (auto& c : cells) {
      posx = 5;
      for (auto& prop : c.prop) {
        p.text( {posx, posy + row / 2}, prop );
        posx += col;
      }
      posy += row;
    }
  }
  
  std::vector<std::string> properties;
  std::vector<cell> cells;
};

template <class T>
struct table_v : view<table_v<T>> {
  
  table_v(T& table) : table{table} {}
  
  auto build(auto&& builder, auto& state) {
    table_widget res {{400, 400}};
    for (auto p : table.properties())
      res.properties.push_back(p);
    for (auto&& e : table.cells) {
      res.cells.push_back({});
      for (auto p : e.properties())
        res.cells.back().prop.push_back(p);
    }
    return res;
  }
  
  rebuild_result rebuild(auto&&... args) {
    return {};
  }
  
  T& table;
};

auto make_view(State& state)
{
  return table_v { state.table };
}

inline void run_app() {
  State state;
  auto app = make_app(state, &make_view);
  app.run(state);
}