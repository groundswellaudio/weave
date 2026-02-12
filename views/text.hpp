#pragma once

#include "views_core.hpp"
#include <string_view>
#include <format>

namespace weave::widgets {

struct text : widget_base
{
  struct properties {
    bool operator==(const properties& o) const = default;
    rgba_u8 color = colors::white;
    float font_size = 11;
  };
  
  private : 
  
  static constexpr float x_margin = 5;
  
  std::string str;
  properties prop;
  float text_width = 0;
  
  public : 
  
  const auto& string() const { return str; }
  
  void set_string(std::string_view new_str, const graphics_context& ctx) {
    set_string(std::string{new_str}, ctx);
  }
  
  void set_string(std::string new_text, const graphics_context& ctx) {
    str = std::move(new_text);
    text_width = ctx.text_bounds(str, font_size()).x;
  }
  
  float font_size() const {
    return prop.font_size;
  }
  
  void set_font_size(float new_size, const graphics_context& ctx) {
    prop.font_size = new_size;
    text_width = ctx.text_bounds(str, font_size()).x;
  }
  
  auto size_info() const {
    widget_size_info res;
    res.min = point{30, prop.font_size + 2 * 5};
    res.nominal_size = point{text_width + 2 * x_margin, prop.font_size + 2 * 5};
    res.flex_factor = point{0.1, 0};
    return res;
  }
  
  void on(ignore, ignore) 
  {
  }
  
  void paint(painter& p) {
    p.font_size(prop.font_size);
    p.fill_style(prop.color);
    p.text_align(text_align::x::left, text_align::y::center);
    p.text({x_margin, size().y / 2}, str);
  }
};

template <class W>
struct with_label : W {
  
  vec2f size() {
    return W::size() + vec2f{text_size, 0};
  }
  
  auto position() const {
    return W::position() - vec2f{text_size, 0};
  }
  
  void set_position(vec2f pos) {
    W::set_position( pos + vec2f{text_size, 0} );
  }
  
  void paint(painter& p) {
    p.font_size(11);
    p.fill_style(colors::white);
    p.text_align(text_align::x::left, text_align::y::center);
    p.text({0, W::size().y / 2}, text);
  }
  
  std::string text;
  float text_size;
};

} // widgets

namespace weave::views {

template <class... Args>
struct text : view<text<Args...>> {
  
  text(std::string_view str, Args... args) : str{str}, fmt_args{args...} {}
  
  //text(std::string_view str) : str{str} {}
  
  text(text&&) = default;
  text(const text&) = default;
  
  vec2f bounds(graphics_context& gctx) {
    return str.size() == 0 ? vec2f{0, prop.font_size} : gctx.text_bounds(str, prop.font_size);
  }
  
  using widget_t = widgets::text;
  
  auto make_string() const {
    std::string string;
    apply([&string, this] (auto&&... elems) { string = std::vformat(str, std::make_format_args(elems...)); }, fmt_args);
    return string;
  }
  
  auto build(const build_context& b, ignore) {
    auto& gctx = b.graphics_context();
    auto res = widget_t{};
    res.set_string(make_string(), gctx);
    return res;
  }
  
  rebuild_result rebuild(text Old, widget_ref w, auto& up, ignore) {
    auto& wb = w.as<widget_t>();
    rebuild_result res {};
    if (str != Old.str || fmt_args != Old.fmt_args) {
      wb.set_string(make_string(), up.context().graphics_context());
      res |= rebuild_result::size_change;
    }
    return res;
  }
  
  widget_t::properties prop;
  std::string_view str;
  tuple<Args...> fmt_args;
};

template <class T, class... Ts>
text(T&& str, Ts&&... ts) -> text<Ts...>;

template <class V>
struct with_label {
  
  auto build(auto&& b, auto& state) {
    return widgets::with_label{view.build(b, state), str};
  }
  
  V view;
  std::string_view str;
};

} // views