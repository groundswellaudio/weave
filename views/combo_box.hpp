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

struct basic_modal_menu : widget_base {
  
  using value_type = unsigned;
  
  static constexpr float row_size = 15;
  
  using vec = std::vector<std::string>; 
  
  basic_modal_menu(float width, vec2f pos, const vec& choices) 
  : widget_base{{width + 2 * 10, choices.size() * row_size}, pos}, 
    choices{choices} 
  { 
  }
  
  void on(mouse_event e, event_context<basic_modal_menu>& ec) 
  {
    if (e.is_mouse_down())
    {
      if (hovered != -1) 
        ec.write(hovered);
      ec.close_modal_menu();
    }
    else if (e.is_mouse_move()) 
    {
      // std::cout << e.position.y << std::endl;
      hovered = e.position.y / row_size;
      if (hovered >= choices.size())
        hovered = -1;
    }
  }
  
  void paint(painter& p, unsigned val) {
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
  
  const std::vector<std::string>& choices;
  int hovered = -1;
};

struct combo_box_widget : widget_base {
  
  using value_type = unsigned;
  
  void on(mouse_event e, event_context<combo_box_widget>& ec) {
    hovered = e.is_mouse_enter() ? true : e.is_mouse_exit() ? false : hovered;
    if (!e.is_mouse_down())
      return;
    auto width = max_text_width(choices, ec.context().graphics_context());
    auto menu = basic_modal_menu{width, {0, size().y}, choices};
    ec.open_modal_menu(with_lens_t{menu, ec.lens.clone()}, this);
  }
  
  void paint(painter& p, unsigned val) {
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 6);
    
    if (hovered) {
      p.fill_style(rgba_f{colors::white}.with_alpha(0.3));
      p.fill_rounded_rect({0, 0}, size(), 6);
    }
    
    p.fill_style(colors::white);
    p.text_align(text_align::x::center, text_align::y::center);
    p.font_size(11);
    p.text( size() / 2, choices[val] );
  }
  
  std::vector<std::string> choices;
  bool hovered = false;
};

namespace views {

template <class Lens, class Range = std::vector<std::string>>
struct combo_box : view<combo_box<Lens, Range>> {
  combo_box(auto l, Range choices) : lens{make_lens(l)}, choices{std::move(choices)} 
  {}
  
  template <class S>
  auto build(auto&& builder, S& state) {
    auto size = vec2f{50, 20};
    if constexpr (^Range == ^std::vector<std::string>) {
      auto w = max_text_width(choices, builder.context().graphics_context());
      return with_lens<S>(combo_box_widget{{w + 10, 20}, choices}, lens);
    }
    else {
      std::vector<std::string> vec;
      if constexpr (^Range == ^audio_devices_range) {
        for (auto h : choices)
          vec.emplace_back(h.name());
      }
      else {
        for (auto&& e : choices)
          vec.emplace_back(e);
      }
      auto w = max_text_width(vec, builder.context().graphics_context());
      return with_lens<S>(combo_box_widget{{w + 10, 20}, std::move(vec)}, lens);
    }
  }
  
  rebuild_result rebuild(combo_box& Old, widget_ref w, ignore, ignore) {
    return {};
  }
  
  Lens lens;
  Range choices;
};

template <class Lens, class Range = std::vector<std::string>>
combo_box(Lens, Range) -> combo_box<make_lens_result<Lens>, Range>;

} // views