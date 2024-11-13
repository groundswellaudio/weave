#pragma once

#include "views_core.hpp"
#include <string_view>
#include "../cursor.hpp"

struct toggle_button_widget
{
  static constexpr float button_radius = 8;
  
  std::string_view str;
  float font_size;
  
  using value_type = bool;
  
  void on(mouse_event e, vec2f sz, event_context<bool> ec) {
    if (e.is_mouse_enter())
      set_mouse_cursor(mouse_cursor::hand);
    else if (e.is_mouse_exit())
      set_mouse_cursor(mouse_cursor::arrow);
    else if (e.is_mouse_down())
      ec.write(!ec.read());
  }
  
  void paint(painter& p, bool flag, vec2f sz) 
  {
    p.fill_style(rgba_f{1.f, 1.f, 0.f, 1.f});
    
    p.circle({button_radius, button_radius}, button_radius);
    
    if (flag)
    {
      p.fill_style(colors::red);
      p.circle({button_radius, button_radius}, 6);
    }
    
    p.fill_style(colors::white);
    p.text_alignment(text_align::x::left, text_align::y::center);
    p.font_size(font_size);
    p.text( {button_radius * 2 + 2, button_radius}, str );
  }
};

template <class Lens>
struct toggle_button : view {
  
  toggle_button(Lens l, std::string_view str) : lens{l}, str{str} {}
  
  auto construct(widget_tree_builder& b, ignore) {
    auto w = b.template create_widget<toggle_button_widget>(lens, {60, 15}, str, font_size);
    view::this_id = w.parent_widget()->id();
    return w;
  }
  
  void rebuild(toggle_button<Lens>& New, widget_tree_updater& b, ignore)
  {
    auto& w = b.consume_leaf().template as<toggle_button_widget>();
    if (New == *this)
      return;
    *this = New;
  }
  
  Lens lens;
  std::string_view str;
  float font_size = 13;
};

template <class State, class Callback>
struct trigger_button_widget {
  
  Callback callback;
  std::string_view str;
  float font_size;
  
  void on(mouse_event e, vec2f this_size, void* state) {
    if (e.is_mouse_enter())
      set_mouse_cursor(mouse_cursor::hand);
    else if (e.is_mouse_exit())
      set_mouse_cursor(mouse_cursor::arrow);
    if (!e.is_mouse_down())
      return;
    callback( *static_cast<State*>(state) );
  }
  
  void paint(painter& p, vec2f sz) 
  {
    //p.stroke_style(colors::black);
    p.font_size(font_size);
    p.fill_style(colors::white);
    p.text_alignment(text_align::x::center, text_align::y::center);
    p.text( {sz.x / 2.f, sz.y / 2.f}, str ); 
  }
  
};

template <class Fn>
struct trigger_button : view {
  
  template <class T>
  trigger_button(T str, Fn fn) : str{str}, fn{fn} {} 
  
  template <class State>
  void construct(widget_tree_builder& b, State& state) {
    // ensure that the callback is well formed
    using z = decltype(fn(state));
    auto next = b.create_widget<trigger_button_widget<State, Fn>>(empty_lens{}, {60, 15}, fn, str, font_size);
    view::this_id = next.parent_widget()->id();
  }
  
  template <class State>
  void rebuild(trigger_button<Fn> New, widget_tree_updater& b, State& s) {  
    auto& w = b.consume_leaf().template as<trigger_button_widget<State, Fn>>();
    if (str == New.str)
      return;
    *this = New;
  }
  
  std::string_view str;
  Fn fn;
  float font_size = 13;
};

template <class T, class Fn>
trigger_button(T, Fn) -> trigger_button<Fn>;

/*
template <class L>
struct toggle_button(L) -> toggle_button<L>; */ 