#pragma once

#include "views_core.hpp"
#include "modifiers.hpp"
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
  text_align::x align;
  
  public : 
  
  void set_alignment(text_align::x val) {
    align = val;
  }
  
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
    res.nominal = point{text_width + 2 * x_margin, prop.font_size + 2 * 5};
    res.flex_factor = point{0.1, 0};
    return res;
  }
  
  void on(ignore, ignore) 
  {
  }
  
  void paint(painter& p) {
    p.font_size(prop.font_size);
    p.fill_style(prop.color);
    p.text_align(align, text_align::y::center);
    float pos_x = align == text_align::x::center 
                  ? size().x / 2 : align == text_align::x::right
                  ? size().x - x_margin : x_margin;
    p.text({pos_x, size().y / 2}, str);
  }
};

template <class W>
struct with_label : widget_base {
  
  static constexpr float margin = 3; 
  
  template <class... Args>
  with_label(Args&&... args) : child{WEAVE_FWD(args)...} {}
  
  auto size_info() const {
    auto base = child.size_info();
    widget_size_info res = child.size_info();
    res.min.x += text_size + margin * 2;
    res.max.x += text_size + margin * 2;
    res.nominal.x += text_size + margin * 2;
    // res.aspect_ratio FIXME : compute the proper aspect ratio if the base has one
    return res;
  }
  
  void layout(point size) {
    child.set_position(point{text_size + margin * 2, 0});
    auto sz = size - point{text_size + margin * 2, 0};
    child.set_size(sz);
  }
  
  void paint(painter& p) {
    p.font_size(11);
    p.fill_style(colors::white);
    p.text_align(text_align::x::left, text_align::y::center);
    p.text({margin, child.size().y / 2}, text);
  }
  
  auto traverse_children(auto fn) {
    return fn(child);
  }
  
  void set_label(std::string str, graphics_context& ctx) {
    text = WEAVE_MOVE(str);
    text_size = ctx.text_bounds(str, 11).x;
  }
  
  void set_label(std::string_view str, graphics_context& ctx) {
    text = std::string{str};
    text_size = ctx.text_bounds(str, 11).x;
  }
  
  void on(mouse_event e, event_context& ec) {}
  
  private : 
  
  std::string text;
  float text_size;
  W child;
};

} // widgets

namespace weave::views {

template <class... Args>
struct text : view<text<Args...>>, view_modifiers {
  
  text(std::string_view str, Args... args) : str{str}, fmt_args{args...} {}
  
  //text(std::string_view str) : str{str} {}
  
  text(text&&) = default;
  text(const text&) = default;
  
  point bounds(graphics_context& gctx) {
    return str.size() == 0 ? point{0, prop.font_size} : gctx.text_bounds(str, prop.font_size);
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
    res.set_alignment(align_x);
    res.set_font_size(prop.font_size, b.graphics_context());
    return res;
  }
  
  rebuild_result rebuild(const text& Old, widget_ref w, const build_context& up, ignore) {
    auto& wb = w.as<widget_t>();
    rebuild_result res {};
    if (str != Old.str || fmt_args != Old.fmt_args) {
      wb.set_string(make_string(), up.graphics_context());
      res |= rebuild_result::size_change;
    }
    if (Old.prop.font_size != prop.font_size) {
      wb.set_font_size(prop.font_size, up.graphics_context());
      res |= rebuild_result::size_change;
    }
    wb.set_alignment(align_x);
    return res;
  }
  
  auto&& align_center(this auto&& self) {
    self.align_x = text_align::x::center;
    return WEAVE_FWD(self);
  }
  
  auto align_right(this auto self) {
    self.align_x = text_align::x::right;
    return WEAVE_FWD(self);
  }
  
  auto&& font_size(this auto&& self, float sz) {
    self.prop.font_size = sz;
    return self;
  }
  
  widget_t::properties prop;
  std::string_view str;
  tuple<Args...> fmt_args;
  text_align::x align_x = text_align::x::left;
};

template <class T, class... Ts>
text(T&& str, Ts&&... ts) -> text<Ts...>;

template <class V>
struct with_label : view<with_label<V>>, view_modifiers {
  
  with_label(std::string_view str, V view) : str{str}, view{WEAVE_MOVE(view)} {}
  
  auto build(const build_context& ctx, auto& state) {
    using underlying = decltype(view.build(ctx, state));
    auto res = widgets::with_label<underlying>{view.build(ctx, state)};
    res.set_label(str, ctx.graphics_context());
    return res;
  }
  
  rebuild_result rebuild(const with_label<V>& old, widget_ref w, const build_context& ctx, auto& state) {
    using underlying = decltype(view.build(ctx, state));
    auto& wl = w.as<widgets::with_label<underlying>>();
    if (str != old.str) {
      wl.set_label(str, ctx.graphics_context());
      return rebuild_result::size_change;
    }
    return {};
  }
  
  std::string_view str;
  V view;
};

} // views