#pragma once

#include "views_core.hpp"
#include "audio.hpp"

#include <ranges>

namespace weave::widgets {

struct popup_menu_stack;

struct popup_menu : widget_base {
  
  static constexpr float row_size = 20;
  static constexpr auto margin = vec2f{10, 4};
  static constexpr float font_size = 13;
  
  struct element {
    std::string name;
    widget_action<optional<popup_menu>(popup_menu*)> action;
    bool is_submenu_opener;
  };
  
  private : 
  
  std::vector<element> elements;
  int hovered = -1;
  bool has_child = false;
  float text_width = 0;
  
  public : 
  
  template <class Str, class Fn>
  void add_element(Str&& str, Fn fn, graphics_context& gctx);
  
  widget_size_info size_info() const {
    widget_size_info res;
    res.min.y = elements.size() * row_size + margin.y * 2;
    res.min.x = text_width + margin.x * 2;
    res.max = res.min;
    res.nominal = res.min;
    res.flex_factor = point{0, 0};
    return res;
  }
  
  void close_children(event_context& ec);
  void open_child(event_context& ec, int idx);
  
  void close(event_context& ec);
  
  bool can_open_submenu() const {
    return std::ranges::any_of(elements, [] (auto& e) { return e.is_submenu_opener;});
  }
  
  void on(mouse_event e, event_context& ec) 
  {
    if (e.is_down())
    {
      if (!rectangle(size()).contains(e.position)) {
        ec.pop_overlay(this);
      }
      else if (hovered != -1 && !elements[hovered].is_submenu_opener)
        elements[hovered].action(ec, this);
    }
    else if (e.is_move()) 
    {
      if (!rectangle(size()).contains(e.position)) {
        hovered = -1;
        return;
      }
      auto next_hovered = (e.position.y - margin.y) / row_size;
      if (next_hovered == hovered)
        return;
      hovered = next_hovered;
      ec.request_repaint();
      if (has_child) {
        close_children(ec);
      }
      if (hovered >= elements.size())
        hovered = -1;
      else if (elements[hovered].is_submenu_opener) {
        open_child( ec, hovered );
      }
    }
  }
  
  void paint(painter& p) {
    p.fill_style(rgb_f(colors::gray) * 0.35);
    p.fill(rounded_rectangle(size()));
    p.stroke_style(colors::black);
    p.stroke(rounded_rectangle(size()));
    float pos = margin.y;
    p.fill_style(colors::white);
    p.font_size(13);
    p.text_align(text_align::x::left, text_align::y::center);
    for (auto& c : elements) {
      p.text( {margin.x, pos + row_size / 2}, c.name );
      pos += row_size;
    }
    
    if (hovered != -1) {  
      p.fill_style(rgba_f{colors::cyan}.with_alpha(0.3));
      auto overlay = rectangle({0, hovered * row_size + margin.y}, {size().x, row_size}); 
      p.fill(overlay);
    }
  }
};

struct popup_menu_stack : widget_base {
  
  widget_size_info size_info() const {
    widget_size_info res;
    res.min = size();
    res.max = size();
    res.nominal = size();
    return res;
  }
  
  void push(popup_menu&& m) {
    menus.push_back(std::move(m));
  }
  
  auto traverse_children(auto&& fn) {
    return std::all_of(menus.begin(), menus.end(), fn);
  }
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_down())
      ec.pop_overlay(this);
  }
  
  void paint(painter& p) {}
  
  std::vector<popup_menu> menus;
};

void popup_menu::close(event_context& ec) {
  auto overlay_ptr = ec.has_parent() && ec.parent().is<popup_menu_stack>() 
                      ? ec.parent() : widget_ref(this);
  ec.pop_overlay(overlay_ptr);
}

template <class Str, class Fn>
void popup_menu::add_element(Str&& str, Fn fn, graphics_context& gctx) {
  constexpr bool is_submenu_opener = std::is_same_v<popup_menu, 
    decltype(std::invoke(fn, std::declval<event_context&>()))
  >;
  auto action = [fn] (event_context& ec, popup_menu* this_) -> optional<popup_menu> {
    if constexpr (is_submenu_opener)
      return std::invoke(fn, ec);
    else
    {
      // We pop the whole popup menu stack on a leaf action
      std::invoke(fn, ec);
      this_->close(ec);
      return {};
    }
  };
  std::string elem_str = str;
  elements.push_back(element{elem_str, action, is_submenu_opener});
  text_width = std::max( gctx.text_bounds(elem_str, font_size).x, text_width );
  set_size(point{text_width + margin.x * 2, elements.size() * row_size + margin.y * 2});
}

void popup_menu::close_children(event_context& ec) {
  auto& p = ec.parent().as<popup_menu_stack>();
  auto idx = this - p.menus.data();
  assert( p.menus.size() > 1 && "popup menu stack contains only one element" );
  ec.request_repaint();
  has_child = false;
  // careful here : we change the address of this
  p.menus.erase(p.menus.begin() + idx + 1, p.menus.end());
}

void popup_menu::open_child(event_context& ec, int idx) {
  assert( elements[idx].is_submenu_opener );
  auto& p = ec.parent().as<popup_menu_stack>();
  auto w = *elements[idx].action(ec, this);
  w.set_position({position().x + size().x, position().y + idx * row_size});
  ec.grab_mouse_focus(this);
  ec.request_repaint();
  has_child = true;
  // careful here : we change the address of this 
  p.push( std::move(w) );
}

inline void enter_popup_menu(event_context& ec, popup_menu m) {
  if (m.can_open_submenu()) {
    ec.push_overlay( popup_menu_stack{{ec.context().window().size()}, {m}} );
  }
  else {
    ec.push_overlay( std::move(m) );
  }
}

inline void enter_popup_menu_relative(event_context& ec, popup_menu m, widget_ref parent) {
  auto pos = ec.absolute_position() + parent.position();
  m.set_position(m.position() + pos);
  enter_popup_menu(ec, std::move(m));
}

} // widgets