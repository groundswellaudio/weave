#pragma once

#include "spiral.hpp"

struct State {
  
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
  	constexpr float first_row = 22;
    constexpr auto row = 15.f;
    p.font_size(13);
    p.fill_style(colors::white);
    p.text_alignment( text_align::x::left, text_align::y::center );
    float pos_x = 0;
    p.stroke_style(colors::white);
    p.line( {0, first_row}, {size().x, first_row}, 1 );
    
    for (auto& prop : properties) {
    	p.line( {pos_x, 0}, {pos_x, size().y}, 1 );
    	p.text( {5 + pos_x, first_row / 2}, prop );
    	pos_x += 100;
    }
    p.stroke_rect({0, 0}, size(), 1);
    
    float pos = first_row;
    for (auto& c : cells) {	
    	float pos_x = 5;
    	for (auto& prop : c.prop) {
			  p.text( {pos_x, pos + row / 2}, prop );
			  pos_x += 100;
    	}
      pos += row;
    }
  }
  
  std::vector<std::string> properties;
  std::vector<cell> cells;
};

template <class T>
struct table_v {
  
  auto build(auto&& builder, auto& state) {
  	table_widget res {{400, 400}};
    for (auto p : table.properties())
      res.properties.push_back(p);
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
  
  T table;
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