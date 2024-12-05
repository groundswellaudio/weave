#pragma once

#include "views_core.hpp"
#include "../util/tuple.hpp"

#include <cmath> // std::pow
#include <cstdlib> // strtod
#include <format>

struct text_field_widget : widget_base {
  
  using Self = text_field_widget;
  using value_type = std::string;
  
  std::string value_str;
  bool editing = false;
  
  void on(mouse_event e, event_context<Self>& Ec) { 
    if (e.is_double_click()) {
      Ec.grab_keyboard_focus(this);
      editing = true;
    }
  }
  
  void on(keyboard_event e, event_context<Self>& Ec) {
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
        Ec.write(value_str);
        break;
      
      default: 
        break;
    }
  }
  
  void paint(painter& p, ignore) {
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

template <class Lens>
struct text_field : view<text_field<Lens>> {
  
  text_field(auto lens) : lens{make_lens(lens)} {}
  
  template <class S>
  auto build(const widget_builder&, S& state) {
    auto init_val = lens.read(state);
    auto res = text_field_widget{{50, 15}, init_val};
    return with_lens<S>(std::move(res), lens);
  }
  
  rebuild_result rebuild(text_field<Lens>& old, widget_ref elem, const widget_updater& up, auto& state) {
    auto& w = elem.as<text_field_widget>();
    auto val = lens.read(state);
    if (!up.context().has_keyboard_focus(elem)) 
      w.value_str = val;
    return {};
  }

  Lens lens;
};

template <class Lens>
text_field(Lens) -> text_field<make_lens_result<Lens>>;

struct numeric_field_properties {
  double min = 0, max = std::numeric_limits<double>::max();
};

struct numeric_field_widget : widget_base {
  
  using Self = numeric_field_widget;
  using value_type = double;
  
  numeric_field_properties prop;
  double value;
  std::string value_str;
  bool accept_decimal;
  
  void on(mouse_event e, event_context<Self>& Ec) {
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
  
  void on(keyboard_event e, event_context<Self>& Ec) {
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
        Ec.write(value);
        break;
      
      default: 
        break;
    }
  }
  
  void paint(painter& p, double) {
    p.fill_style(colors::black);
    p.rectangle({0, 0}, size());
    p.fill_style(colors::white);
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6);
    p.text_align(text_align::x::center, text_align::y::center);
    p.text(size() / 2, value_str);
  }
};

template <class Lens>
struct numeric_field : view<numeric_field<Lens>> {
  
  numeric_field(auto lens) : lens{make_lens(lens)} {}
  
  template <class S>
  auto build(const widget_builder&, S& state) {
    auto init_val = lens.read(state);
    auto res = numeric_field_widget{{50, 15}, prop, (double)init_val};
    res.update_str();
    res.accept_decimal = is_floating_point(type_of(^init_val));
    return with_lens<S>(std::move(res), lens);
  }
  
  rebuild_result rebuild(numeric_field<Lens>& old, widget_ref elem, const widget_updater& up, auto& state) {
    auto& w = elem.as<numeric_field_widget>();
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

struct numeric_dial_widget : widget_base 
{
  using prop_t = numeric_field_properties;
  
  numeric_dial_widget(prop_t prop) : prop{prop} {}
  
  numeric_field_properties prop;
  float mult_mod;
  
  using Self = numeric_field_widget;
  using value_type = float;
  
  void on(mouse_event e, event_context<Self>& ec) {
    if (e.is_mouse_down())
      mult_mod = (1.f - 3 * e.position.x / size().x);
    else if (e.is_mouse_drag())
    {
      auto delta = - std::pow(10, mult_mod) * e.mouse_drag_delta().y;
      auto NewVal = std::clamp<float>( ec.read() + delta, prop.min, prop.max );
      ec.write( NewVal );
    }
  }
  
  void paint(painter& p, float value) {
    p.stroke_style(colors::red);
    p.stroke_rect({0, 0}, size());
    p.font_size(13.f);
    auto str = std::format("{:.2f}", value);
    p.text_align(text_align::x::center, text_align::y::center);
    p.text( sz / 2, str );
  }
  
  auto layout() const { return vec2f{50, 30}; }
};

template <class L>
using numeric_dial = simple_view_for<numeric_dial_widget, L, numeric_field_properties>;