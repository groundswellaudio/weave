#pragma once

#include "views_core.hpp"
#include "audio.hpp"
#include "dialogs.hpp"

#include <ranges>

namespace weave {

namespace views { 
  template <class Lens, class Range>
  struct combo_box;
}

namespace widgets {

struct combo_box : widget_base {
  
  private : 
  
  template <class L, class R>
  friend struct views::combo_box; 

  static constexpr float margin = 5;
  std::vector<std::string> choices;
  write_fn<int> write;
  int active = 0;
  bool hovered = false;
  
  public : 
  
  combo_box(point size, std::vector<std::string> choices)
  : widget_base{size}, choices{std::move(choices)} {}
  
  float max_width;
  
  vec2f min_size() const { return {30, 15}; }
  vec2f max_size() const { return {max_width, 15.f}; }
  
  void select(event_context& ec, int choice) {
    active = choice;
    write(ec, active);
  }
  
  void on(mouse_event e, event_context& ec) {
    hovered = e.is_enter() ? true : e.is_exit() ? false : hovered;
    if (!e.is_down())
      return;
    popup_menu m;
    int k = 0;
    for (auto& c : choices) 
      m.add_element(c, [this, p = k++] (event_context& ec) { this->select(ec, p); });
    m.update_size(ec.context().graphics_context());
    m.set_position(vec2f{0, size().y});
    enter_popup_menu_relative(ec, std::move(m), this);
    hovered = false;
  }
  
  void paint(painter& p) {
    p.stroke_style(colors::white);
    p.stroke(rounded_rectangle(size()));
    
    if (hovered) {
      p.fill_style(rgba_f{colors::white}.with_alpha(0.3));
      p.fill(rounded_rectangle(size()));
    }
    
    p.fill_style(colors::white);
    p.text_align(text_align::x::center, text_align::y::center);
    p.font_size(11);
    p.text_bounded( size() / 2, size().x, choices[active] );
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
    if constexpr (std::is_same_v<Range, Res>)
      return (choices);
    else if constexpr (std::is_same_v<Range, audio_devices_range>) {
      Res vec;
      for (auto h : choices)
        vec.emplace_back(h.name());
      return vec;
    }
    else return Res{choices.begin(), choices.end()};
  }
  
  template <class S>
  auto build_impl(const build_context& builder, S& state) {
    auto size = vec2f{50, 20};
    auto&& vec = make_string_vec();
    auto w = max_text_width(vec, builder.context().graphics_context(), 11);
    auto res = widget_t{{w + 2 * widget_t::margin, 20}, std::forward<StringVec>(vec)};
    res.max_width = w + 20; 
    return res;
  }
  
  template <class S>
  auto build(const build_context& builder, S& state) {
    auto res = build_impl(builder, state);
    res.write = [f = lens] (event_context& ec, int val) {
      f.write(ec.state<S>(), val);
    };
    res.active = lens.read(state);
    return res;
  }
  
  rebuild_result rebuild(combo_box& Old, widget_ref w, auto& ctx, auto& state) {
    auto& wb = w.as<widget_t>();
    auto val = lens.read(state);
    if (val != wb.active)
      wb.active = val;
    auto new_choices = make_string_vec();
    if (new_choices != wb.choices) {
      wb.choices = std::move(new_choices);
      auto old_width = wb.max_width;
      wb.max_width = max_text_width(wb.choices, ctx.context().graphics_context(), 11) 
                          + 20;
      if (old_width != wb.max_width)
        return rebuild_result::size_change;
    }
    wb.choices = make_string_vec();
    return {};
  }
  
  Lens lens;
  Range choices;
};

template <class Lens, class Range = std::vector<std::string>>
combo_box(Lens, Range) -> combo_box<make_lens_result<Lens>, Range>;

} // views

} // weave