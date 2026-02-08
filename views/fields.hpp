#pragma once

#include "views_core.hpp"
#include "../util/tuple.hpp"

#include <cmath> // std::pow
#include <cstdlib> // strtod
#include <format>

namespace weave::widgets {

static constexpr vec2f min_size_field = {50, 15};

struct text_field : widget_base {
  
  using Self = text_field;
  
  widget_action<std::string_view> write;
  
  private : 
  
  std::string value_str;
  glyph_positions glyph_pos;
  unsigned edit_pos = 0;
  bool show_caret : 1 = false;
  bool editing : 1 = false;
  
  public : 
  
  void set_editing(bool is_edited) { editing = is_edited; }
  
  bool is_being_edited() const { return editing; }
  
  void set_value(std::string new_value) {
    value_str = new_value; 
    edit_pos = value_str.size();
  }
  
  const std::string& value() const { return value_str; }
  
  void on(mouse_event e, event_context& Ec) { 
    if (e.is_double_click()) {
      Ec.grab_keyboard_focus(this);
      editing = true;
      Ec.animate(*this, [] (auto& s) { 
        s.show_caret = !s.show_caret;
        return s.editing; 
      }, 1000);
    }
  }
  
  widget_size_info size_info() const {
    widget_size_info res;
    res.min = min_size_field;
    res.nominal_size = point{100, 15};
    res.max.y = 15;
    res.flex_factor = point{1, 0};
    return res;
  }
  
  void on(keyboard_event e, event_context& Ec) {
    if (e.is_up())
      return;
    
    if (auto C = to_character(e.key))
    {
      value_str.push_back(C);
      Ec.request_repaint();
      return;
    }
    
    switch(e.key) {
      case keycode::backspace:
        value_str.pop_back();
        Ec.request_repaint();
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
    p.fill_style(editing ? colors::red : colors::black);
    p.fill(rectangle(size()));
    p.fill_style(colors::white);
    p.stroke_style(colors::white);
    p.stroke(rounded_rectangle(size(), 3));
    p.text_align(text_align::x::left, text_align::y::center);
    p.text({5, size().y / 2}, value_str);
    
    if (show_caret) {
      // p.line( )
    }
  }
};

} // widgets

namespace weave::views {

template <class Lens>
struct text_field : view<text_field<Lens>> {
  
  text_field(auto lens) : lens{make_lens(lens)} {}
  
  using widget_t = widgets::text_field;
  
  template <class S>
  auto build(const build_context&, S& state) {
    auto init_val = lens.read(state);
    auto res = widget_t{{50, 15}, init_val};
    res.write = to_widget_write(lens); 
    return res;
  }
  
  rebuild_result rebuild(text_field<Lens>& old, widget_ref elem, const build_context& up, auto& state) {
    auto& w = elem.as<widget_t>();
    auto val = lens.read(state);
    if (!up.context().has_keyboard_focus(elem))
      w.set_value(val);
    return {};
  }
  
  void destroy(widget_ref wr, application_context& ctx) {
    auto& w = wr.as<widget_t>();
    if (w.is_being_edited())
      ctx.deanimate(wr);
  }

  Lens lens;
};

template <class Lens>
text_field(Lens) -> text_field<make_lens_result<Lens>>;

} // views

struct numeric_field_properties {
  double min = 0, max = std::numeric_limits<double>::max();
};

namespace weave::widgets {

struct numeric_field : widget_base {
  
  using Self = numeric_field;
  
  numeric_field_properties prop;
  double value;
  std::string value_str;
  bool accept_decimal;
  widget_action<double> write;
  
  void on(mouse_event e, event_context& Ec) {
    if (e.is_double_click()) {
      Ec.grab_keyboard_focus(this);
    }
  }
  
  widget_size_info size_info() const {
    widget_size_info res;
    res.min = min_size_field;
    res.nominal_size = point{100, 15};
    res.max.y = 15;
    res.flex_factor = point{1, 0};
    return res;
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
    if (e.is_up())
      return;
    
    if (is_number(e.key)) {
      value_str.push_back( to_character(e.key) );
      Ec.request_repaint();
      return;
    }
    
    if (to_character(e.key) == '.' && accept_decimal)
    {
      if (value_str.find('.') == std::string::npos) {
        value_str.push_back('.');
        Ec.request_repaint();
      }
      return;
    }
    
    switch(e.key) {
      case keycode::backspace:
        value_str.pop_back();
        Ec.request_repaint();
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
    p.fill(area());
    p.stroke_style(colors::white);
    p.stroke(rounded_rectangle(size()));
    p.fill_style(colors::white);
    p.text_align(text_align::x::center, text_align::y::center);
    p.text(size() / 2, value_str);
  }
};

} // widgets

namespace weave::views {

template <class Lens>
struct numeric_field : view<numeric_field<Lens>> {
  
  using widget_t = widgets::numeric_field;
  
  numeric_field(auto lens) : lens{make_lens(lens)} {}
  
  template <class S>
  auto build(const build_context&, S& state) {
    auto init_val = lens.read(state);
    auto res = widget_t{{50, 15}, prop, (double)init_val};
    res.update_str();
    res.accept_decimal = std::is_floating_point_v<decltype(init_val)>;
    res.write = [a = lens] (event_context& ec, double value) {
      ec.request_rebuild();
      a.write(ec.state<S>(), value);
    };
    return res;
  }
  
  rebuild_result rebuild(numeric_field<Lens>& old, widget_ref elem, const build_context& up, auto& state) {
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

namespace weave::widgets {

struct numeric_dial : widget_base 
{
  using prop_t = numeric_field_properties;
  
  numeric_dial(prop_t prop) : prop{prop} {}
  
  numeric_field_properties prop;
  float mult_mod;
  float value;
  write_fn<float> write;
  
  widget_size_info size_info() const {
    widget_size_info res;
    res.min = min_size_field;
    res.nominal_size = point{75, 15};
    res.max.y = 15;
    res.flex_factor = point{1, 0};
    return res;
  }
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_down())
      mult_mod = (1.f - 3 * e.position.x / size().x);
    else if (e.is_drag())
    {
      auto delta = - std::pow(10, mult_mod) * e.drag_delta().y;
      auto NewVal = std::clamp<float>( value + delta, prop.min, prop.max );
      write(ec, NewVal);
      value = NewVal;
    }
  }
  
  void paint(painter& p) {
    p.stroke_style(colors::white);
    p.stroke(rounded_rectangle(size()));
    p.font_size(13.f);
    p.fill_style(colors::white);
    auto str = std::format("{:.2f}", value);
    p.text_align(text_align::x::center, text_align::y::center);
    p.text( sz / 2, str );
  }
  
  auto layout() const { return vec2f{50, 30}; }
};

} // widgets

namespace weave::views {

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