#pragma once

#include "../util/vec.hpp"
#include "../util/variant.hpp"
#include <string>

namespace weave {

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

struct file_drop_event {
  std::string filename;
};

struct mouse_event {
  vec2f position;
  variant<mouse_enter, mouse_exit, mouse_down, mouse_up, mouse_move, mouse_scroll, file_drop_event> event; 
  
  template <class T>
  bool is() const { return holds_alternative<T>(event); } 
  
  template <class T>
  auto& get_as(this auto&& self) { return get<T>(self.event); }
  
  bool is_double_click() const { return is_down() && get_as<mouse_down>().double_click; }
  vec2f scroll_delta() const { return get_as<mouse_scroll>().delta; }
  bool is_scroll() const { return is<mouse_scroll>(); }
  bool is_enter() const { return is<mouse_enter>(); }
  bool is_exit() const { return is<mouse_exit>(); }
  bool is_move() const { return is<mouse_move>(); }
  auto drag_delta() const { return get_as<mouse_move>().delta; }
  bool is_drag() const { return is<mouse_move>() && get_as<mouse_move>().is_dragging; }
  bool is_down() const { return is<mouse_down>(); }
  bool is_up() const { return is<mouse_up>(); }
  bool is_right_click() const { 
    return is<mouse_down>() && get_as<mouse_down>().button == mouse_button::right; 
  }
  
  bool is_file_drop() const { return is<file_drop_event>(); }
  auto& dropped_file() const { return get_as<file_drop_event>().filename; }
};

template <class T, class... Ts>
static constexpr auto is_one_of = ((std::is_same_v<T, Ts>) or ...);

template <class T>
concept is_mouse_event = is_one_of<T, mouse_enter, mouse_exit, mouse_down, mouse_up, mouse_move, mouse_scroll>;

} // weave 