#pragma once

#include <string_view>
#include <functional>
#include <concepts>

#include "views_core.hpp"
#include "../cursor.hpp"

struct button_properties {
  bool operator==(const button_properties&) const = default; 
  std::string_view str;
  float font_size = 13;
  rgba_u8 active_color = rgba_u8{colors::cyan}.with_alpha(255 * 0.5);
};

static constexpr rgba_u8 button_overlay_color = rgba_u8{colors::white}.with_alpha(75);
static constexpr float button_margin = 5;

namespace widgets {

struct toggle_button : widget_base
{
  static constexpr float button_radius = 8;
  
  button_properties prop;
  write_fn<bool> write;
  bool value;
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_mouse_enter())
      set_mouse_cursor(mouse_cursor::hand);
    else if (e.is_mouse_exit())
      set_mouse_cursor(mouse_cursor::arrow);
    else if (e.is_mouse_down()) {
      write(ec, !value);
      value = !value;
    }
  }
  
  void paint(painter& p) 
  {
    auto sz = size();
    p.fill_style(colors::white);
    
    p.stroke_style(rgba_f{colors::white}.with_alpha(0.5));
    p.stroke_rounded_rect({0, 0}, sz, 6, 1);
    
    // p.circle({button_radius, button_radius}, button_radius);
    
    if (value)
    {
      p.fill_style(prop.active_color);
      p.fill_rounded_rect({0, 0}, sz, 6);
      //p.circle({button_radius, button_radius}, 6);
    } 
    
    p.fill_style(colors::white);
    p.text_align(text_align::x::center, text_align::y::center);
    p.font_size(sz.y - 3);
    p.text(sz / 2.f, prop.str);
  }
};

struct trigger_button : widget_base {
  
  std::string_view str;
  float font_size;
  widget_action<void()> on_click;
  bool disabled = false;
  bool hovered = false;
  
  void on(mouse_event e, event_context& ec) {
    if (disabled)
      return;
    if (e.is_mouse_enter())
      hovered = true;
    else if (e.is_mouse_exit())
      hovered = false;
    if (!e.is_mouse_down())
      return;
    on_click(ec);
  }
  
  void set_disabled(bool new_disabled) {
    disabled = new_disabled;
    if (disabled)
      hovered = false;
  }
  
  void paint(painter& p) 
  {
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6, 1);
    
    if (hovered) {
      p.fill_style(button_overlay_color);
      p.fill_rounded_rect({0, 0}, size(), 6);
    }
    
    p.font_size(font_size);
    p.fill_style(!disabled ? rgba{colors::white} : rgba{colors::white}.with_alpha(110));
    p.text_align(text_align::x::left, text_align::y::center);
    p.text( {button_margin, sz.y / 2.f}, str ); 
  }
};

} // widgets

namespace views {

template <class Lens>
struct toggle_button : view<toggle_button<Lens>> {
  
  using widget_t = widgets::toggle_button;
  
  toggle_button(std::string_view str, Lens l) : lens{l} {
    properties.str = str;
  }
  
  template <class S>
  auto build(widget_builder b, S& s) {
    auto value = lens.read(s);
    return widget_t{{50, 20}, properties, lens_write_for_widget(lens), value};
  }
  
  rebuild_result rebuild(toggle_button<Lens>& Old, widget_ref w, ignore, ignore) {
    return {};
  }
  
  Lens lens;
  button_properties properties;
};

template <class Fn>
struct trigger_button : view<trigger_button<Fn>> {
  
  using widget_t = widgets::trigger_button;
  
  template <class T>
  trigger_button(T str, Fn fn) : str{str}, fn{fn} {} 
  
  auto text_bounds(application_context& ctx) {
    return ctx.graphics_context().text_bounds(str, font_size);
  }
  
  template <class State>
  auto build(const widget_builder& b, State& s) {
    auto sz = text_bounds(b.context());
    decltype(widget_t::on_click) action;
    if constexpr ( requires (event_context& ec) { std::invoke(fn, ec); } )
      action = [f = fn] (auto& ec) { std::invoke(f, ec); };
    else {
      action = [f = fn] (auto& ec) { std::invoke(f, ec.template state<State>()); };
      //using t = decltype(fn(std::declval<event_context&>()));
      //static_assert( requires (event_context& ec) { std::invoke(fn, ec); } );
    }
    auto size = sz + vec2f{button_margin, button_margin} * 2;
    auto res = widget_t{{size}};
    res.str = str;
    res.on_click = std::move(action);
    res.font_size = font_size;
    res.disabled = disabled;
    return res;
  }
  
  template <class State>
  rebuild_result rebuild(trigger_button<Fn>& Old, widget_ref w, auto&& up, State& s) {
    auto& b = w.as<widget_t>();
    b.set_disabled(disabled);
    b.str = str;
    b.font_size = font_size;
    return {};
  }
  
  auto& disable_if(bool flag) {
    disabled = flag;
    return *this;
  }
  
  std::string_view str;
  Fn fn;
  float font_size = 11;
  bool disabled = false;
};

template <class T, class Fn>
trigger_button(T, Fn) -> trigger_button<Fn>; 

} // views

namespace widgets
{

template <class PaintFn>
struct graphic_toggle_button : widget_base {
  
  PaintFn paint_fn;
  write_fn<bool> write;
  bool flag;
  bool hovered = false;
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_mouse_enter())
      hovered = true;
    else if (e.is_mouse_exit())
      hovered = false;
    else if (e.is_mouse_down()) {
      flag = !flag;
      write(ec, flag);
    }
  }
  
  void paint(painter& p) {
    std::invoke(paint_fn, p, flag, size());
    if (hovered) {
      p.fill_style(button_overlay_color);
      p.rounded_rectangle({0, 0}, size(), 6);
    }
  }
};

template <class PaintFn>
struct graphic_trigger_button : widget_base {
  
  PaintFn paint_fn;
  widget_action<void()> on_click;
  
  bool hovered = false;
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_mouse_enter())
      hovered = true;
    else if (e.is_mouse_exit())
      hovered = false;
    else if (e.is_mouse_down())
      on_click(ec);
  }
  
  void paint(painter& p) {
    std::invoke(paint_fn, p, size());
    if (hovered) {
      p.fill_style(button_overlay_color);
      p.rounded_rectangle({0, 0}, size(), 6);
    }
  }
};

} // widgets

namespace views 
{
  template <class PaintFn, class WriteFn>
  struct graphic_toggle_button : view<graphic_toggle_button<PaintFn, WriteFn>> {
    
    graphic_toggle_button (PaintFn P, bool val, WriteFn W) 
    : val{val}, paint_fn{P}, write_fn{W} {}
    
    template <class S>
    auto build(const widget_builder& b, S& state) {
      widgets::graphic_toggle_button<PaintFn> res {{size_v}, paint_fn, {}, val};
      res.write = [w = write_fn] (event_context& ec, bool v) -> bool {
        return std::invoke(w, ec.state<S>(), v);
      };
      res.flag = val;
      return res;
    }
    
    rebuild_result rebuild(auto& Old, widget_ref w, ignore, ignore) {
      auto& wb = w.as<widgets::graphic_toggle_button<PaintFn>>();
      wb.flag = val;
      return {};
    }
    
    auto& size(vec2f sz) {
      size_v = sz;
      return *this;
    }
    
    bool val;
    vec2f size_v = {20, 20};
    PaintFn paint_fn;
    WriteFn write_fn;
  };
  
  template <class PaintFn, class Payload>
  struct graphic_trigger_button : view<graphic_trigger_button<PaintFn, Payload>> {
    
    graphic_trigger_button (PaintFn P, Payload W) 
    : paint_fn{P}, payload{W} {}
    
    template <class S>
    auto build(const widget_builder& b, S& state) {
      widgets::graphic_trigger_button<PaintFn> res {{size_v}, paint_fn, {}};
      res.on_click = [w = payload] (event_context& ec) {
        std::invoke(w, ec.state<S>());
      };
      return res;
    }
    
    rebuild_result rebuild(ignore Old, widget_ref w, ignore, ignore) {
      return {};
    }
    
    auto& size(vec2f sz) {
      size_v = sz;
      return *this;
    }
    
    vec2f size_v = {20, 20};
    PaintFn paint_fn;
    Payload payload;
  };
} // views