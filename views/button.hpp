#pragma once

#include "views_core.hpp"
#include <string_view>
#include "../cursor.hpp"

struct button_properties {
  bool operator==(const button_properties&) const = default; 
  std::string_view str;
  float font_size = 13;
  rgba_u8 active_color = rgba_u8{colors::cyan}.with_alpha(255 * 0.5);
};

struct toggle_button_widget
{
  static constexpr float button_radius = 8;
  
  button_properties prop;
  
  using value_type = bool;
  
  void on(mouse_event e, event_context<toggle_button_widget> ec) {
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
    
    p.stroke_style(rgba_f{colors::white}.with_alpha(0.5));
    p.stroke_rounded_rect({0, 0}, sz, 6, 1);
    
    // p.circle({button_radius, button_radius}, button_radius);
    
    if (flag)
    {
      p.fill_style(prop.active_color);
      p.fill_rounded_rect({0, 0}, sz, 6);
      //p.circle({button_radius, button_radius}, 6);
    } 
    
    p.fill_style(colors::white);
    p.text_alignment(text_align::x::center, text_align::y::center);
    p.font_size(sz.y - 3);
    p.text(sz / 2.f, prop.str);
  }
};

template <class Lens>
struct toggle_button : view<toggle_button<Lens>> {
  
  toggle_button(Lens l, std::string_view str) : lens{l} {
    properties.str = str;
  }
  
  auto build(widget_builder b, ignore) {
    return with_lens{ toggle_button_widget{properties}, dyn_lens<bool>{lens} };
  }
  
  void rebuild(toggle_button<Lens>& New, widget_ref w, ignore, ignore) {
    if (New.properties == this->properties)
      return;
    *this = New;
  }
  
  Lens lens;
  button_properties properties;
};

/* 
template <class State, class Callback>
struct trigger_button_widget {
  
  using value_type = void;
  
  Callback callback;
  std::string_view str;
  float font_size;
  
  void on(mouse_event e, event_context<void> ec) {
    if (e.is_mouse_enter())
      set_mouse_cursor(mouse_cursor::hand);
    else if (e.is_mouse_exit())
      set_mouse_cursor(mouse_cursor::arrow);
    if (!e.is_mouse_down())
      return;
    callback( *static_cast<State*>(ec.application_state()) );
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
struct trigger_button : view<trigger_button<Fn>> {
  
  template <class T>
  trigger_button(T str, Fn fn) : str{str}, fn{fn} {} 
  
  template <class State>
  void build(widget_builder& b, State& state) {
    // ensure that the callback is well formed
    using z = decltype(fn(state));
    return make_tuple( trigger_button_widget<State, Fn>{fn, str, font_size}, 
                       empty_lens{}, 
                       widget_ctor_args{b.next_id(), {60.f, 15.f}} );
  }
  
  template <class State>
  void rebuild(trigger_button<Fn> New, widget& w, State& s) {  
    if (str == New.str)
      return;
    *this = New;
  }
  
  std::string_view str;
  Fn fn;
  float font_size = 13;
};

template <class T, class Fn>
trigger_button(T, Fn) -> trigger_button<Fn>; */ 

/*
template <class L>
struct toggle_button(L) -> toggle_button<L>; */ 