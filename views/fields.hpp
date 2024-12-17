#pragma once

#include "views_core.hpp"
#include "../util/tuple.hpp"

#include <cmath> // std::pow
#include <cstdlib> // strtod
#include <format>

namespace widgets {

struct text_field : widget_base {
  
  using Self = text_field;
  
  write_fn<std::string_view> write;
  std::string value_str;
  bool editing = false;
  
  void on(mouse_event e, event_context& Ec) { 
    if (e.is_double_click()) {
      Ec.grab_keyboard_focus(this);
      editing = true;
    }
  }
  
  void on(keyboard_event e, event_context& Ec) {
    if (e.is_key_up())
      return;
    
    if (auto C = to_character(e.key))
    {
      value_str.push_back(C);
      return;
    }
    
    switch(e.key) {
      case keycode::backspace:
        value_str.pop_back();
        break;
      case keycode::enter: 
        Ec.release_keyboard_focus();
        editing = false;
        write(Ec, value_str);
        break;
      
      default: 
        break;
    }
  }
  
  void paint(painter& p) {
    p.fill_style(colors::black);
    p.rectangle({0, 0}, size());
    p.fill_style(colors::white);
    p.stroke_style(colors::white);
    if (editing) {
      p.fill_style(colors::red);
      p.rectangle({0, 0}, size());
      p.fill_style(colors::white);
    }
    p.stroke_rounded_rect({0, 0}, size(), 6);
    p.text_align(text_align::x::center, text_align::y::center);
    p.text(size() / 2, value_str);
  }
};

} // widgets

namespace views {

template <class Lens>
struct text_field : view<text_field<Lens>> {
  
  text_field(auto lens) : lens{make_lens(lens)} {}
  
  using widget_t = widgets::text_field;
  
  template <class S>
  auto build(const widget_builder&, S& state) {
    auto init_val = lens.read(state);
    auto res = widget_t{{50, 15}, init_val};
    res.write = to_widget_write(lens); 
    return res;
  }
  
  rebuild_result rebuild(text_field<Lens>& old, widget_ref elem, const widget_updater& up, auto& state) {
    auto& w = elem.as<widget_t>();
    auto val = lens.read(state);
    if (!up.context().has_keyboard_focus(elem))
      w.value_str = val;
    return {};
  }

  Lens lens;
};

template <class Lens>
text_field(Lens) -> text_field<make_lens_result<Lens>>;

} // views

struct numeric_field_properties {
  double min = 0, max = std::numeric_limits<double>::max();
};

namespace widgets {

struct numeric_field : widget_base {
  
  using Self = numeric_field;
  
  numeric_field_properties prop;
  double value;
  std::string value_str;
  bool accept_decimal;
  write_fn<double> write;
  
  void on(mouse_event e, event_context& Ec) {
    if (e.is_double_click()) {
      Ec.grab_keyboard_focus(this);
    }
  }
  
  void update_from_str() {
    auto from_str = strtod(value_str.data(), nullptr);
    value = std::clamp(from_str, prop.min, prop.max);
    if (value != from_str)
      update_str();
  }
  
  void update_str() {
    value_str = std::format("{}", value);
  }
  
  void on(keyboard_event e, event_context& Ec) {
    if (e.is_key_up())
      return;
    
    if (is_number(e.key)) {
      value_str.push_back( to_character(e.key) );
      return;
    }
    
    if (to_character(e.key) == '.' && accept_decimal)
    {
      if (value_str.find('.') == std::string::npos)
        value_str.push_back('.');
      return;
    }
    
    switch(e.key) {
      case keycode::backspace:
        value_str.pop_back();
        break;
      case keycode::enter: 
        update_from_str();
        Ec.release_keyboard_focus();
        write(Ec, value);
        break;
      
      default: 
        break;
    }
  }
  
  void paint(painter& p) {
    p.fill_style(colors::black);
    p.rectangle({0, 0}, size());
    p.fill_style(colors::white);
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6);
    p.text_align(text_align::x::center, text_align::y::center);
    p.text(size() / 2, value_str);
  }
};

} // widgets

namespace views {

template <class Lens>
struct numeric_field : view<numeric_field<Lens>> {
  
  using widget_t = widgets::numeric_field;
  
  numeric_field(auto lens) : lens{make_lens(lens)} {}
  
  template <class S>
  auto build(const widget_builder&, S& state) {
    auto init_val = lens.read(state);
    auto res = widget_t{{50, 15}, prop, (double)init_val};
    res.update_str();
    res.accept_decimal = is_floating_point(type_of(^init_val));
    res.write = [a = lens] (event_context& ec, double value) {
      ec.request_rebuild();
      a.write(ec.state<S>(), value);
    };
    return res;
  }
  
  rebuild_result rebuild(numeric_field<Lens>& old, widget_ref elem, const widget_updater& up, auto& state) {
    auto& w = elem.as<widget_t>();
    auto val = lens.read(state);
    if (!up.context().has_keyboard_focus(elem))
    {
      w.value = val;
      w.update_str();
    }
    return {};
  }
  
  auto& range(double min, double max) {
    prop.min = min;
    prop.max = max;
    return *this;
  }

  Lens lens;
  numeric_field_properties prop;
};

template <class Lens>
numeric_field(Lens) -> numeric_field<make_lens_result<Lens>>;

} // views

namespace widgets {

struct numeric_dial : widget_base 
{
  using prop_t = numeric_field_properties;
  
  numeric_dial(prop_t prop) : prop{prop} {}
  
  numeric_field_properties prop;
  float mult_mod;
  float value;
  write_fn<float> write;
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_mouse_down())
      mult_mod = (1.f - 3 * e.position.x / size().x);
    else if (e.is_mouse_drag())
    {
      auto delta = - std::pow(10, mult_mod) * e.drag_delta().y;
      auto NewVal = std::clamp<float>( value + delta, prop.min, prop.max );
      write(ec, NewVal);
      value = NewVal;
    }
  }
  
  void paint(painter& p) {
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6);
    p.font_size(13.f);
    p.fill_style(colors::white);
    auto str = std::format("{:.2f}", value);
    p.text_align(text_align::x::center, text_align::y::center);
    p.text( sz / 2, str );
  }
  
  auto layout() const { return vec2f{50, 30}; }
};

} // widgets

namespace views {

template <class L>
struct numeric_dial : simple_view_for<widgets::numeric_dial, L, numeric_field_properties> {
  
  numeric_dial(L lens, numeric_field_properties prop = {}) 
    : simple_view_for<widgets::numeric_dial, L, numeric_field_properties>{lens, prop} {} 
    
  auto& range(double min, double max) {
    this->min = min; 
    this->max = max;
    return *this;
  }
};

} // viwes