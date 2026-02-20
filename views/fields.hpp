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
  unsigned cursor_index[2] = {unsigned(-1), 0}; // left-right, cursor_index[1] is the caret
  bool show_caret : 1 = false;
  bool editing : 1 = false;
  
  void update_glyph_pos(graphics_context& gc) {
    gc.get_glyph_positions(glyph_pos, value_str, {5, size().y / 2});
  }
  
  void set_caret_position_from(point pos)
  {
    if (value_str.size() == 0)
      return;
    // this set the right cursor
    cursor_index[1] = glyph_pos.index_from_pos(pos.x);
  }
  
  void set_selection_from(point pos)
  {
    if (value_str.size() == 0)
      return;
    
    auto idx = glyph_pos.index_from_pos(pos.x);
    
    if (idx > cursor_index[1])
      cursor_index[1] = idx;
    else if (idx < cursor_index[0])
      cursor_index[0] = idx;
  }
  
  auto& caret()             { return cursor_index[1]; }
	const auto& caret() const { return cursor_index[1]; }
	
	bool has_selection() const { return cursor_index[0] != unsigned(-1); }
	
  public : 
  
  void enter_editing(event_context& ec) { 
    editing = true;
    ec.grab_keyboard_focus(this);
    ec.animate(*this, [] (auto& s, ignore) { 
      s.show_caret = !s.show_caret;
      return s.editing; 
    }, 800);
    cursor_index[1] = value_str.size();
    cursor_index[0] = -1;
  }
  
  void exit_editing(event_context& ec) {
    ec.release_keyboard_focus();
    ec.deanimate(this);
    editing = false;
    show_caret = false;
    ec.request_repaint();
  }
  
  bool is_being_edited() const { return editing; }
  
  void set_value(std::string new_value, graphics_context& gc) {
    value_str = new_value; 
    cursor_index[1] = value_str.size();
    update_glyph_pos(gc);
  }
  
  const std::string& value() const { return value_str; }
  
  void on(mouse_event e, event_context& Ec) { 
    if (e.is_double_click()) 
      enter_editing(Ec);
    else if (e.is_down()) {
      cursor_index[0] = -1;
      set_caret_position_from(e.position);
      Ec.request_repaint();
    }
    else if (e.is_exit()) 
      exit_editing(Ec);
    else if (e.is_drag()) {
      set_selection_from(e.position);
      Ec.request_repaint();
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
    
    if (e.is_text_input()) {
      if (has_selection()) // replace selection with input
      {
        value_str.replace( value_str.begin() + cursor_index[0], value_str.begin() + cursor_index[1],
                e.text_input().begin(), e.text_input().end() );
        cursor_index[1] = cursor_index[0] + 1;
        cursor_index[0] = (unsigned)(-1);
      }
      else // insert after caret
      {
        auto input = e.text_input();
        value_str.insert( value_str.begin() + caret(), input.begin(), input.end() );
        caret() += input.size();
      }
      update_glyph_pos(Ec.graphics_context());
      Ec.request_repaint();
      return;
    }
    
    switch(e.key()) {
      case keycode::backspace: {
        if (caret() == 0)
          return;
        if (has_selection())
        {
          value_str.erase(value_str.begin() + cursor_index[0], value_str.begin() + cursor_index[1]);
          cursor_index[1] = cursor_index[0];
          cursor_index[0] = unsigned(-1);
        }
        else
        {
          value_str.erase( value_str.begin() + caret() - 1 );
          --caret();
        }
        Ec.request_repaint();
        break;
      }
      case keycode::enter: 
        exit_editing(Ec);
        write(Ec, value_str);
        break;
      case keycode::arrow_left:
        if (has_selection()) {
          cursor_index[1] = cursor_index[0];
          cursor_index[0] = unsigned(-1);
        }
        else {
          if (caret() == 0)
            return;
          --caret();
        }
        Ec.request_repaint();
        break;
      case keycode::arrow_right:
        if (has_selection())
          cursor_index[0] = unsigned(-1);
        else {
          if (caret() == value_str.size())
            return;
          ++caret();
        }
        Ec.request_repaint();
        break;
      default: 
        break;
    }
  }
  
  void paint(painter& p) {
    p.fill_style(editing ? rgb_u8{40, 40, 40} : colors::black);
    p.fill(rectangle(size()));
    p.fill_style(colors::white);
    p.stroke_style(colors::white);
    p.stroke(rounded_rectangle(size(), 3));
    p.text_align(text_align::x::left, text_align::y::center);
    p.text({5, size().y / 2}, value_str);
    
    if (has_selection()) {
      auto cursor_pos_l = glyph_pos.pos_from_index(cursor_index[0]);
      auto cursor_pos_r = glyph_pos.pos_from_index(cursor_index[1]);
      rectangle r { {cursor_pos_l, 0}, {cursor_pos_r - cursor_pos_l, size().y} };
      p.fill_style( rgba_f{colors::cyan}.with_alpha(0.3) );
      p.fill(r);
    }
    
    if (show_caret) {
      auto px = glyph_pos.pos_from_index(caret());
      p.line( point{px, 0}, point{px, size().y}, 2 );
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
    if (!up.application_context().has_keyboard_focus(elem))
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
    
    if (is_number(e.key())) {
      value_str.push_back( to_character(e.key()) );
      Ec.request_repaint();
      return;
    }
    
    if (to_character(e.key()) == '.' && accept_decimal)
    {
      if (value_str.find('.') == std::string::npos) {
        value_str.push_back('.');
        Ec.request_repaint();
      }
      return;
    }
    
    switch(e.key()) {
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
    if (!up.application_context().has_keyboard_focus(elem))
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