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
	static constexpr float row = 15;
	
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
	
	void handle_mouse_down_body(vec2f pos, application_context& ctx) {
		int selected = (pos.y - first_row) / row;
		if (selected >= cells.size())
			return;
		bool select_range = ctx.is_active(key_modifier::shift);
		
		auto first_selected = visit( [] (auto& elem) {
			using E = %remove_reference(type_of(^elem));
			if constexpr ( ^E == ^int )
				return elem;
			else if constexpr ( ^E == ^vec2<int, int> ) 
				return elem.x;
			else
				return elem.front();
		}, selection);
		
		auto& sr = get<0>(selection);
		if (select_range) {
			auto first = 
			auto& sr = selection.emplace<1>();
			sr.m0 = std::min(i, first_selected);
			sr.m1 = std::max(i, first_selected);
		}
		else {
			cells[selected].selected = true;
			sr.m0 = selected;
		}
	}
	
  void on(mouse_event e, event_context<table_widget>& Ec) 
  {
  	if (e.is_mouse_down()) {
  	  if (e.position.y < first_row) 
  	    handle_mouse_down_header(e.position);
  	  else 
  	  	handle_mouse_down_body(e.position, ec.context());
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
  
  void paint(painter& p) {
  	constexpr float first_row = 22;
    constexpr auto row = 15.f;
    p.font_size(13);
    p.fill_style(colors::white);
    p.text_alignment( text_align::x::left, text_align::y::center );
    float pos_x = 0;
    p.stroke_style(colors::white);
    p.line( {0, first_row}, {size().x, first_row}, 1 );
    
    for (auto& [prop, posx] : properties) {
    	p.line( {posx, 0}, {posx, size().y}, 1 );
    	p.text( {5 + posx, first_row / 2}, prop );
    }
    p.stroke_rect({0, 0}, size(), 1);
    
    float pos = first_row;
    for (auto& c : cells) {	
    	if (c.selected) {
    		p.fill_style(rgba{colors::cyan}.with_alpha(70));
    		p.rectangle({0, pos}, {size().x, row});
    		p.fill_style(colors::white);
    	}
    	int k = 0;
    	for (auto& prop : c.prop) {
			  p.text( {5 + properties[k++].m1, pos + row / 2}, prop );
    	}
	    pos += row;
    }
  }
  
	variant<tuple<int, int>, std::vector<int>> selection;
	
  std::vector<tuple<std::string, float>> properties;
  std::vector<cell> cells;
  
  int dragging = -1;
};

template <class T>
struct table_v {
  
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