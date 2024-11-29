#pragma once

#include "vec.hpp"
#include "../util/variant.hpp"

struct mouse_enter {};

struct mouse_exit {};

enum class mouse_button : unsigned char {
	right,
	middle,
	left
};

struct mouse_down
{
	bool is_double_click() const { return double_click; }
	
	mouse_button button;
	bool double_click;
};

struct mouse_up {
	mouse_button button;
};

struct mouse_move {
	vec2f delta;
	bool is_dragging;
	mouse_button button;
};

struct mouse_scroll {
	vec2f delta;
};

struct mouse_event {
  vec2f position;
  variant<mouse_enter, mouse_exit, mouse_down, mouse_up, mouse_move, mouse_scroll> event; 
  
  template <class T>
  bool is() const { return holds_alternative<T>(event); } 
  
  template <class T>
  auto& get_as(this auto&& self) { return get<T>(self.event); }
  
  bool is_double_click() const { return is_mouse_down() && get_as<mouse_down>().double_click; }
  vec2f mouse_scroll_delta() const { return get_as<mouse_scroll>().delta; }
  bool is_mouse_scroll() const { return is<mouse_scroll>(); }
  bool is_mouse_enter() const { return is<mouse_enter>(); }
  bool is_mouse_exit() const { return is<mouse_exit>(); }
  bool is_mouse_move() const { return is<mouse_move>(); }
  auto mouse_drag_delta() const { return get_as<mouse_move>().delta; }
  bool is_mouse_drag() const { return is<mouse_move>() && get_as<mouse_move>().is_dragging; }
  bool is_mouse_down() const { return is<mouse_down>(); }
  bool is_mouse_up() const { return is<mouse_up>(); }
  bool is_right_click() const { 
    return is<mouse_down>() && get_as<mouse_down>().button == mouse_button::left; 
  }
};

template <class T, class... Ts>
static constexpr auto is_one_of = ((^T == ^Ts) or ...);

template <class T>
concept is_mouse_event = is_one_of<T, mouse_enter, mouse_exit, mouse_down, mouse_up, mouse_move, mouse_scroll>;