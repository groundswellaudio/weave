#pragma once

#include <string_view>
#include <functional>
#include <concepts>

#include "views_core.hpp"
#include "../cursor.hpp"

namespace weave {

struct button_properties {
  bool operator==(const button_properties&) const = default; 
  std::string_view str;
  float font_size = 13;
  rgba_u8 active_color = rgba_u8{colors::cyan}.with_alpha(255 * 0.5);
};

static constexpr rgba_u8 button_overlay_color = rgba_u8{colors::white}.with_alpha(75);
static constexpr float button_margin = 5;

}

namespace weave::widgets {

struct toggle_button : widget_base
{
  static constexpr float button_radius = 8;
  
  button_properties prop;
  widget_action<bool> write;
  bool value;
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_enter())
      set_mouse_cursor(mouse_cursor::hand);
    else if (e.is_exit())
      set_mouse_cursor(mouse_cursor::arrow);
    else if (e.is_down()) {
      write(ec, !value);
      value = !value;
    }
  }
  
  widget_size_info size_info() const {
    return {{20, 15}};
  }
  
  void paint(painter& p) 
  {
    auto sz = size();
    p.fill_style(colors::white);
    
    p.stroke_style(rgba_f{colors::white}.with_alpha(0.5));
    p.stroke(rounded_rectangle(sz), 1);
    
    p.stroke(rounded_rectangle(sz));
    
    // p.circle({button_radius, button_radius}, button_radius);
    
    if (value)
    {
      p.fill_style(prop.active_color);
      p.fill(rounded_rectangle(sz, 6));
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
  float text_width = 20;
  
  static constexpr point margin = {10, 4};
  
  vec2f min_size() const { return point{10, font_size} + 2 * margin; }
  vec2f max_size() const { return point{text_width, font_size} + 2 * margin; }
  
  void on(mouse_event e, event_context& ec) {
    if (disabled)
      return;
    if (e.is_enter()) 
      (hovered = true, ec.request_repaint());
    else if (e.is_exit())
      (hovered = false, ec.request_repaint());
    if (!e.is_down())
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
    p.stroke(rounded_rectangle(size()), 1);
    
    if (hovered) {
      p.fill_style(button_overlay_color);
      p.fill(rounded_rectangle(size()));
    }
    
    p.font_size(font_size);
    p.fill_style(!disabled ? rgba{colors::white} : rgba{colors::white}.with_alpha(110));
    p.text_align(text_align::x::center, text_align::y::center);
    p.text( {sz.x / 2, sz.y / 2.f}, str ); 
  }
};

} // widgets

namespace weave::views {

template <class Lens>
struct toggle_button : view<toggle_button<Lens>> {
  
  using widget_t = widgets::toggle_button;
  
  toggle_button(std::string_view str, Lens l) : lens{l} {
    properties.str = str;
  }
  
  template <class S>
  auto build(build_context b, S& s) {
    auto value = lens.read(s);
    return widget_t{{50, 20}, properties, lens_write_for_widget(lens), value};
  }
  
  rebuild_result rebuild(toggle_button<Lens>& Old, widget_ref w, ignore, ignore) {
    return {};
  }
  
  Lens lens;
  button_properties properties;
};

/// The view of a trigger button.
template <class Fn>
struct button : view<button<Fn>> {
  
  using widget_t = widgets::trigger_button;
  
  template <class T>
  button(T str, Fn fn) : str{str}, fn{fn} {} 
  
  button(const button&) = default;
  
  auto text_bounds(application_context& ctx) {
    return ctx.graphics_context().text_bounds(str, font_size);
  }
  
  template <class State>
  auto build(const build_context& b, State& s) {
    auto sz = text_bounds(b.context());
    decltype(widget_t::on_click) action = [f = fn] (auto& ec) { context_invoke<State>(f, ec); };
    auto size = sz + vec2f{button_margin, button_margin} * 2;
    auto res = widget_t{{size}};
    res.text_width = size.x;
    res.str = str;
    res.on_click = std::move(action);
    res.font_size = font_size;
    res.disabled = disabled;
    return res;
  }
  
  template <class State>
  rebuild_result rebuild(button<Fn>& Old, widget_ref w, auto&& up, State& s) {
    auto& b = w.as<widget_t>();
    rebuild_result res;
    b.set_disabled(disabled);
    bool update_bounds = false; 
    if (b.str != str) {
      b.str = str;
      update_bounds = true;
    }
    if (font_size != b.font_size) {
      b.font_size = font_size;
      update_bounds = true;
    }
    if (update_bounds) {
      auto sz = text_bounds(up.context());
      b.text_width = sz.x;
      res |= rebuild_result::size_change;
    }
    b.on_click = [f = fn] (auto& ec) { context_invoke<State>(f, ec); };
    return {};
  }
  
  auto& disable_if(bool flag) {
    disabled = flag;
    return *this;
  }
  
  private : 
  
  std::string_view str;
  Fn fn;
  float font_size = 11;
  bool disabled = false;
};

template <class T, class Fn>
button(T, Fn) -> button<Fn>;

} // views

namespace weave::widgets
{

template <class PaintFn>
struct graphic_toggle_button : widget_base {
  
  PaintFn paint_fn;
  widget_action<bool(bool)> write;
  bool flag;
  bool hovered = false;
  
  vec2f min_size() const { return size(); }
  vec2f max_size() const { return size(); }
  
  // vec2f expand_factor() const { return {0, 0}; }
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_enter())
      (hovered = true, ec.request_repaint());
    else if (e.is_exit())
      (hovered = false, ec.request_repaint());
    else if (e.is_down()) 
      flag = write(ec, !flag);
  }
  
  void paint(painter& p) {
    std::invoke(paint_fn, p, flag, size());
    if (hovered) {
      p.fill_style(button_overlay_color);
      p.fill( rounded_rectangle(size()) );
    }
  }
};

template <class PaintFn>
struct graphic_trigger_button : widget_base {
  
  PaintFn paint_fn;
  widget_action<void()> on_click;
  
  bool hovered = false;
  
  vec2f min_size() const { return size(); }
  vec2f max_size() const { return size(); }
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_enter())
      (hovered = true, ec.request_repaint());
    else if (e.is_exit())
      (hovered = false, ec.request_repaint());
    else if (e.is_down())
      on_click(ec);
  }
  
  void paint(painter& p) {
    std::invoke(paint_fn, p, size());
    if (hovered) {
      p.fill_style(button_overlay_color);
      p.fill(rounded_rectangle(size()));
    }
  }
};

} // widgets

namespace weave::views 
{
  template <class PaintFn, class WriteFn>
  struct graphic_toggle_button : view<graphic_toggle_button<PaintFn, WriteFn>> {
    
    graphic_toggle_button (PaintFn P, bool val, WriteFn W) 
    : val{val}, paint_fn{P}, write_fn{W} {}
    
    template <class S>
    auto build(const build_context& b, S& state) {
      widgets::graphic_toggle_button<PaintFn> res {{size_v}, paint_fn, {}, val};
      res.write = [w = write_fn] (event_context& ec, bool v) -> bool {
        return context_invoke<S>(w, ec, v);
      };
      res.flag = val;
      return res;
    }
    
    rebuild_result rebuild(auto& Old, widget_ref w, ignore, ignore) {
      auto& wb = w.as<widgets::graphic_toggle_button<PaintFn>>();
      wb.flag = val;
      return {};
    }
    
    auto& size(point sz) {
      size_v = sz;
      return *this;
    }
    
    bool val;
    point size_v = {20, 20};
    PaintFn paint_fn;
    WriteFn write_fn;
  };
  
  template <class PaintFn, class Payload>
  struct graphic_button : view<graphic_button<PaintFn, Payload>> {
    
    graphic_button (PaintFn P, Payload W) 
    : paint_fn{P}, payload{W} {}
    
    using widget_t = widgets::graphic_trigger_button<PaintFn>;
    
    template <class S>
    auto build(const build_context& b, S& state) {
      widget_t res {{size_v}, paint_fn, {}};
      res.on_click = action<S>();
      return res;
    }
    
    template <class S>
    rebuild_result rebuild(ignore Old, widget_ref w, ignore, S& state) {
      auto& b = w.as<widget_t>();
      b.on_click = action<S>();
      return {};
    }
    
    auto& size(vec2f sz) {
      size_v = sz;
      return *this;
    }
    
    private : 
    
    template <class S>
    auto action() const { 
      return [w = payload] (event_context& ec) {
        context_invoke<S>(w, ec);
      };
    }
    
    point size_v = {20, 20};
    PaintFn paint_fn;
    Payload payload;
  };
} // views