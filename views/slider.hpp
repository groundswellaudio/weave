#pragma once

#include "views_core.hpp"
#include <format>

namespace weave {

struct slider_properties {
  bool operator==(const slider_properties&) const = default;
  float min = 0, max = 1;
  rgba_u8 background_color = rgb_u8{30, 30, 30};
  rgba_u8 active_color = rgba_f{colors::cyan}.with_alpha(0.3);
  rgba_u8 text_color = colors::white;
  unsigned short precision = 2; 
};

/// A function and its inverse
struct scalar_space {
  
  static float identity (float x) { return x; }
  
  float operator()(float x) const { return fn(x); }
  
  static scalar_space exponential() {
    return {(float(*)(float))&std::exp, (float(*)(float))&log};
  }
  
  static scalar_space logarithmic() {
    return { (float(*)(float)) &std::log, (float(*)(float)) &std::exp };
  }
  
  static scalar_space log10() {
    return {[] (float x) { return std::log(x) / std::log(10); }, 
            [] (float x) { return std::exp(x * std::log(10)); }};
  }
  
  static scalar_space square_root() {
    return { static_cast<float(*)(float)>(&std::sqrt), [] (float x) { 
      auto r = x * x;
      return (x > 0) ? r : -r;
    }};
  }
  
  static scalar_space cubic_root() {
    return {
      [] (float x) { return std::pow(x, 1.f / 3.f); }, 
      [] (float x) { return x * x * x; }
    };
  }
  
  static scalar_space decibel() {
    return {
      [] (float x) { return 20 * std::log(x) / std::log(10); },
      [] (float x) { return std::exp(x * std::log(10) * 20); }
    };
  }
  
  std::function<float(float)> fn = &identity, inverse = &identity;
};

} // weave

namespace weave::widgets 
{

struct slider : widget_base
{
  slider_properties prop;
  float ratio_val; 
  float scaled_val; 
  std::string value_str;
  write_fn<float> write;
  scalar_space space;
  
  /// Whether to write the scaled value or the ratio value
  bool write_scaled = true;
  
  using EvCtx = event_context;
  
  auto size_info() const {
    widget_size_info res;
    res.min = point{30, 15};
    res.nominal_size = point{50, 15};
    res.flex_factor = point{0.5, 0};
    return res;
  }
  
  /// Set the ratio value and return the scaled value
  float set_ratio(float new_ratio) {
    auto delta = ratio_val - new_ratio;
    // auto df = space((ratio_val + new_ratio) / 2);
    ratio_val = new_ratio;
    
    auto scaled = scaled_value();
    value_str = std::format("{:.{}f}", scaled, prop.precision);
    return scaled;
  }
  
  /// Set the scaled value.
  void set_scaled(float value) {
    auto min_inv = space.inverse(prop.min);
    auto new_ratio = (space.inverse(value) - min_inv) / (space.inverse(prop.max) - min_inv);
    set_ratio(new_ratio);
  }
  
  /// Minmax of the scaled value
  void set_range(float min, float max) {
    prop.min = min;
    prop.max = max;
  }
  
  /// Set the function used to compute the scaled value
  void set_value_space(const scalar_space& s) {
    space = s;
  }
  
  /// Return the value which is displayed as a string
  float scaled_value() const {
    auto min_inv = space.inverse(prop.min);
    return space(min_inv + (space.inverse(prop.max) - min_inv) * ratio_val);
  }
  
  /// Return the ratio of the slider bar (between 0 and 1)
  float ratio_value() const {
    return ratio_val;
  }
  
  void on(mouse_event e, EvCtx& ec) 
  {
    if (!e.is_drag() && !e.is_down())
      return;
    
    auto sz = size();
    float ratio = std::min(e.position.x / sz.x, 1.f);
    ratio = std::max(0.f, ratio);
    
    auto written_val = write_scaled ? scaled_value() : ratio; 
    write(ec, written_val);
    set_ratio(ratio);
    ec.request_repaint();
  }
  
  void paint(painter& p) 
  {
    auto sz = size();
    p.fill_style(prop.background_color);
    p.fill(rounded_rectangle(sz));
    
    p.fill_style(prop.active_color);
    p.fill(rounded_rectangle({ratio_value() * sz.x, sz.y}));
    
    static constexpr auto outline_col = rgba_f{colors::white}.with_alpha(0.5);
    p.stroke_style(outline_col);
    p.stroke(rounded_rectangle(sz));
    
    p.fill_style(colors::white);
    p.text_align(text_align::x::center, text_align::y::center);
    p.font_size(11);
    p.text({sz.x / 2, sz.y / 2}, value_str);
  }
};

} // widgets

namespace weave::views {

template <class Lens>
struct slider : view<slider<Lens>> {
  
  template <class L>
  slider(L lens) : lens{make_lens(lens)} {}
  
  using widget_t = widgets::slider;
  
  void set_widget_value(widget_t& w, float val) {
    if (write_scaled_v)
      w.set_scaled(val);
    else
      w.set_ratio(val);
  }
  
  template <class S>
  auto build(const build_context& b, S& state) {
    auto val = lens.read(state);
    auto res = widget_t{{size}, properties};
    res.write = [l = lens] (event_context& c, float val) { l.write(c.state<S>(), val); };
    set_widget_value(res, val);
    res.write_scaled = write_scaled_v;
    res.space = space_v;
    return res;
  }
  
  template <class S>
  rebuild_result rebuild(slider<Lens>& Old, widget_ref wb, build_context up, S& state) {
    auto& w = wb.as<widget_t>();
    auto val = lens.read(state);
    if (properties != Old.properties) {
      w.prop = properties;
    }
    w.write_scaled = write_scaled_v;
    w.space = space_v;
    if (val != write_scaled_v ? w.scaled_value() : w.ratio_value()) {
      set_widget_value(w, val);
    }
    if (size != Old.size) {
      w.set_size(size);
      return rebuild_result::size_change;
    }
    return {};
  }
  
  auto& range(float min, float max) {
    properties.min = min;
    properties.max = max;
    return *this;
  }
  
  auto& space(const scalar_space& s) {
    space_v = s;
    return *this;
  }
  
  auto& exponential() {
    space_v = scalar_space::exponential();
    return *this;
  }
  
  auto& logarithmic() {
    space_v = scalar_space::logarithmic();
    return *this;
  }
  
  auto& write_scaled(bool v) {
    write_scaled_v = v;
    return *this;
  }
  
  /// Set the numbers of decimals to be displayed
  auto& precision(unsigned prec) {
    properties.precision = prec;
    return *this;
  }
  
  void destroy(widget_ref w) {}
  
  slider_properties properties;
  Lens lens;
  vec2f size = {80, 15};
  scalar_space space_v;
  bool write_scaled_v = true;
};

template <class Lens>
slider(Lens) -> slider<make_lens_result<Lens>>;

} // views