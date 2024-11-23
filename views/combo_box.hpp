#pragma once

#include "views_core.hpp"
#include <ranges>

struct basic_modal_menu : widget_base {
  
  using value_type = unsigned;
  
  static constexpr float row_size = 15;
  
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
      hovered = e.position.y / row_size;
      if (hovered >= choices.size())
        hovered = -1;
    }
  }
  
  void paint(painter& p, unsigned val) {
    p.fill_style(colors::white);
    p.rectangle({0, 0}, size());
    float pos = 2;
    p.fill_style(colors::black);
    p.text_alignment(text_align::x::left, text_align::y::center);
    p.font_size(11);
    for (auto& c : choices) {
      p.text( {2, pos + 11 / 2}, c );
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

struct combo_box : widget_base {
  
  using value_type = unsigned;
  
  vec2f layout() {
    return {50, 20};
  }
  
  void on(mouse_event e, event_context<combo_box>& ec) {
    if (!e.is_mouse_down())
      return;
    auto height = choices.size() * basic_modal_menu::row_size;
    ec.open_modal_menu(basic_modal_menu{{{50, (float)height}, {0, size().y}}, choices}, this);
  }
  
  void paint(painter& p, unsigned val) {
    p.stroke_style(colors::white);
    p.stroke_rounded_rect({0, 0}, size(), 1);
    
    p.fill_style(colors::white);
    p.text_alignment(text_align::x::center, text_align::y::center);
    p.font_size(11);
    p.text( size() / 2, choices[val] );
  }
  
  std::vector<std::string> choices;
};

template <class Lens, class Range = std::vector<std::string>>
struct combo_box_v : view<combo_box_v<Lens, Range>> {
  combo_box_v(Lens l, Range choices) : lens{l}, choices{std::move(choices)} 
  {}
  
  template <class S>
  auto build(auto&& builder, S& state) {
    return with_lens<S>(combo_box{{}, choices}, lens);
  }
  
  rebuild_result rebuild(combo_box_v& Old, widget_ref w, ignore, auto& state) {
    return {};
  }
  
  Lens lens;
  Range choices;
};