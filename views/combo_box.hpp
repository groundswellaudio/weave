#pragma once

#include "views_core.hpp"
#include "audio.hpp"

#include <ranges>

float max_text_width(const std::vector<std::string>& vec, graphics_context& ctx) {
  float res;
  for (auto& s : vec) 
    res = std::max( ctx.text_bounds(s, 11).x, res );
  return res;
}

namespace widgets {

struct basic_modal_menu : widget_base {
  
  using value_type = unsigned;
  using vec = std::vector<std::string>; 
  static constexpr float row_size = 15;
  
  const vec& choices;
  int hovered = -1;
  widget_action<void(int)> on_select;
  
  basic_modal_menu(float width, vec2f pos, const vec& choices) 
  : widget_base{{width + 2 * 10, choices.size() * row_size}, pos}, 
    choices{choices} 
  {
  }
  
  void on(mouse_event e, event_context& ec) 
  {
    if (e.is_mouse_down())
    {
      if (hovered != -1) 
        on_select(ec, hovered);
      ec.pop_overlay();
    }
    else if (e.is_mouse_move()) 
    {
      // std::cout << e.position.y << std::endl;
      hovered = e.position.y / row_size;
      if (hovered >= choices.size())
        hovered = -1;
    }
  }
  
  void paint(painter& p) {
    p.fill_style(rgb_f(colors::gray) * 0.35);
    p.fill_rounded_rect({0, 0}, size(), 6);
    p.stroke_style(colors::black);
    p.stroke_rounded_rect({0, 0}, size(), 6);
    float pos = 2;
    p.fill_style(colors::white);
    p.text_align(text_align::x::left, text_align::y::center);
    p.font_size(11);
    for (auto& c : choices) {
      p.text( {10, pos + 11 / 2}, c );
      pos += row_size;
    }
    
    if (hovered != -1) {  
      p.fill_style(rgba_f{colors::cyan}.with_alpha(0.3));
      p.rectangle({0, hovered * row_size}, {size().x, row_size});
    }
  }
};

struct combo_box : widget_base {

  static constexpr float margin = 5;
  
  std::vector<std::string> choices;
  write_fn<int> write;
  int active = 0;
  bool hovered = false;
  
  void set_value(int x) {
    active = x;
  }
  
  void on(mouse_event e, event_context& ec) {
    hovered = e.is_mouse_enter() ? true : e.is_mouse_exit() ? false : hovered;
    if (!e.is_mouse_down())
      return;
    auto width = max_text_width(choices, ec.context().graphics_context());
    auto menu = basic_modal_menu{width, {0, size().y}, choices};
    menu.on_select = [this] (event_context& ec, int val) {
      active = val;
      write(ec, val);
    };
    
    ec.push_overlay_relative(std::move(menu));
  }
  
  void paint(painter& p) {
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6);
    
    if (hovered) {
      p.fill_style(rgba_f{colors::white}.with_alpha(0.3));
      p.fill_rounded_rect({0, 0}, size(), 6);
    }
    
    p.fill_style(colors::white);
    p.text_align(text_align::x::center, text_align::y::center);
    p.font_size(11);
    p.text( size() / 2, choices[active] );
  }
};

} // widgets

namespace views {

template <class Lens, class Range = std::vector<std::string>>
struct combo_box : view<combo_box<Lens, Range>> {
  
  combo_box(auto l, Range choices) : lens{make_lens(l)}, choices{std::move(choices)} 
  {}
  
  using widget_t = widgets::combo_box;
  
  using StringVec = std::vector<std::string>;
  
  decltype(auto) make_string_vec() {
    using Res = StringVec;
    if constexpr (^Range == ^Res) 
      return (choices);
    else if constexpr (^Range == ^audio_devices_range) {
      Res vec;
      for (auto h : choices)
        vec.emplace_back(h.name());
      return vec;
    }
    else return Res{choices.begin(), choices.end()};
  }
  
  template <class S>
  auto build_impl(const widget_builder& builder, S& state) {
    auto size = vec2f{50, 20};
    auto&& vec = make_string_vec();
    auto w = max_text_width(vec, builder.context().graphics_context());
    return widget_t{{w + 2 * widget_t::margin, 20}, std::forward<StringVec>(vec)};
  }
  
  template <class S>
  auto build(const widget_builder& builder, S& state) {
    auto res = build_impl(builder, state);
    res.write = [f = lens] (event_context& ec, int val) {
      f.write(ec.state<S>(), val);
    };
    res.set_value(lens.read(state));
    return res;
  }
  
  rebuild_result rebuild(combo_box& Old, widget_ref w, ignore, auto& state) {
    auto& wb = w.as<widget_t>();
    auto val = lens.read(state);
    if (val != wb.active)
      wb.set_value(val);
    return {};
  }
  
  Lens lens;
  Range choices;
};

template <class Lens, class Range = std::vector<std::string>>
combo_box(Lens, Range) -> combo_box<make_lens_result<Lens>, Range>;

} // views