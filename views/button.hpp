#pragma once

#include <string_view>
#include <functional>

#include "views_core.hpp"
#include "../cursor.hpp"

struct button_properties {
  bool operator==(const button_properties&) const = default; 
  std::string_view str;
  float font_size = 13;
  rgba_u8 active_color = rgba_u8{colors::cyan}.with_alpha(255 * 0.5);
};

static constexpr rgba_u8 button_overlay_color = rgba_u8{colors::white}.with_alpha(75);

struct toggle_button_widget : widget_base
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
  
  void paint(painter& p, bool flag) 
  {
    auto sz = size();
    p.fill_style(colors::white);
    
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
    p.text_align(text_align::x::center, text_align::y::center);
    p.font_size(sz.y - 3);
    p.text(sz / 2.f, prop.str);
  }
};

namespace views {

template <class Lens>
struct toggle_button : view<toggle_button<Lens>> {
  
  toggle_button(std::string_view str, Lens l) : lens{l} {
    properties.str = str;
  }
  
  template <class S>
  auto build(widget_builder b, S& s) {
    return with_lens<S>(toggle_button_widget{{50, 20}, properties}, lens);
  }
  
  rebuild_result rebuild(toggle_button<Lens>& Old, widget_ref w, ignore, ignore) 
  {
    return {};
  }
  
  Lens lens;
  button_properties properties;
};

} // views

static constexpr float button_margin = 5;

template <class State, class Callback>
struct trigger_button_widget : widget_base {
  
  using value_type = void;
  
  Callback callback;
  std::string_view str;
  float font_size;
  bool disabled = false;
  bool hovered = false;
  
  void on(mouse_event e, event_context<trigger_button_widget<State, Callback>> ec) {
    if (disabled)
      return;
    if (e.is_mouse_enter())
      hovered = true;
    else if (e.is_mouse_exit())
      hovered = false;
    if (!e.is_mouse_down())
      return;
    std::invoke( callback, *static_cast<State*>(ec.state()) );
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

namespace views {

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
    return trigger_button_widget<State, Fn>{ {sz + vec2f{button_margin, button_margin} * 2}, fn, str, font_size, disabled};
  }
  
  template <class State>
  rebuild_result rebuild(trigger_button<Fn>& Old, widget_ref w, auto&& up, State& s) {
    auto& b = w.as<trigger_button_widget<State, Fn>>();
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
  std::function<bool(event_context_t<void>&, bool)> write;
  bool flag;
  bool hovered = false;
  
  void on(mouse_event e, event_context_t<void>& ec) {
    if (e.is_mouse_enter())
      hovered = true;
    else if (e.is_mouse_exit())
      hovered = false;
    else if (e.is_mouse_down())
      flag = write(ec, !flag);
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
  std::function<void(event_context_t<void>&)> on_click;
  bool hovered = false;
  
  void on(mouse_event e, event_context_t<void>& ec) {
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
      res.write = [w = write_fn] (event_context_t<void>& ec, bool v) -> bool {
        return std::invoke(w, *static_cast<S*>(ec.state()), v);
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
      res.on_click = [w = payload] (event_context_t<void>& ec) {
        std::invoke(w, *static_cast<S*>(ec.state()));
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
}