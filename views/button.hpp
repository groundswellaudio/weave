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
  
  template <class S>
  auto build(widget_builder b, S& s) {
    return with_lens<S>(toggle_button_widget{properties}, lens);
  }
  
  rebuild_result rebuild(toggle_button<Lens>& Old, widget_ref w, ignore, ignore) 
  {
    return {};
  }
  
  Lens lens;
  button_properties properties;
};

static constexpr float button_margin = 5;

template <class State, class Callback>
struct trigger_button_widget : widget_base {
  
  using value_type = void;
  
  Callback callback;
  std::string_view str;
  float font_size;
  bool hovered = false;
  
  void on(mouse_event e, event_context<trigger_button_widget<State, Callback>> ec) {
    if (e.is_mouse_enter())
      hovered = true;
    else if (e.is_mouse_exit())
      hovered = false;
    if (!e.is_mouse_down())
      return;
    callback( *static_cast<State*>(ec.state()) );
  }
  
  void paint(painter& p) 
  {
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6, 1);
    
    if (hovered) {
      p.fill_style(rgba_u8{colors::white}.with_alpha(75));
      p.fill_rounded_rect({0, 0}, size(), 6);
    }
    
    p.font_size(font_size);
    p.fill_style(colors::white);
    p.text_alignment(text_align::x::left, text_align::y::center);
    p.text( {button_margin, sz.y / 2.f}, str ); 
  }
};

template <class Fn>
struct trigger_button : view<trigger_button<Fn>> {
  
  template <class T>
  trigger_button(T str, Fn fn) : str{str}, fn{fn} {} 
  
  auto text_bounds(application_context& ctx) {
    return ctx.graphics_context().text_bounds(str, font_size);
  }
  
  template <class State>
  auto build(const widget_builder& b, State& s) {
    auto sz = text_bounds(b.context());
    return trigger_button_widget<State, Fn>{ {sz + vec2f{button_margin, button_margin} * 2}, fn, str, font_size};
  }
  
  template <class State>
  rebuild_result rebuild(trigger_button<Fn>& Old, widget_ref w, auto&& up, State& s) {
    return {};
  }
  
  std::string_view str;
  Fn fn;
  float font_size = 11;
};

template <class T, class Fn>
trigger_button(T, Fn) -> trigger_button<Fn>; 