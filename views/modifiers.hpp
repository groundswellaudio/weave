#pragma once

#include "views_core.hpp"
#include "popup_menu.hpp"
#include "../core/widget.hpp"
#include <functional>

namespace weave::widgets {

template <class W>
struct event_listener : W {
  
  widget_action<bool(input_event)> fn; 
  
  using base_t = W;
  
  auto& base() { return (W&)*this; }
  
  void on(mouse_event e, event_context& ec) {
    if (fn(ec, e))
      return;
    base().on(e, ec);
  }
  
  void on(keyboard_event e, event_context& ec) {
    if (fn(ec, e))
      return;
    if constexpr ( requires {base().on(e, ec);} ) 
      base().on(e, ec);
  }
};

template <class W>
struct with_fixed_size : W {
  
  widget_size_info size_info() const {
    widget_size_info res;
    res.min = W::size();
    res.max = W::size();
    res.nominal = W::size();
    res.flex_factor = point{0, 0};
    return res;
  }
  
};

template <class W>
struct with_nominal_size : W {
  
  widget_size_info size_info() const {
    widget_size_info res = W::size_info();
    res.nominal = nominal_size;
    return res;
  }
  
  point nominal_size;
};

template <class T, class B>
struct with_background : T {
  
  void do_layout(point sz) {
    background.do_layout(sz);
    T::do_layout(sz);
  }
  
  void paint(painter& p) {
    background.paint(p);
    T::paint(p);
  }
  
  B background;
};

template <class T>
struct with_flex_factor : T {
  
  widget_size_info size_info() const {
    auto res = T::size_info();
    res.flex_factor = flex_factor;
    return res;
  }
  
  point flex_factor;
};

template <class W>
struct with_popup_menu : W {
  
  void on(mouse_event e, event_context& ec) {
    if (e.is_right_click())
      enter_popup_menu_relative(ec, popup_opener(ec), this);
    else
      W::on(e, ec);
  }
  
  void on_child_event(mouse_event e, widget_ref r, event_context& ec) {
    if (e.is_right_click())
      enter_popup_menu_relative(ec, popup_opener(ec), this);
    else if constexpr (is_child_event_listener<W>)
      W::on_child_event(e, r, ec);
  }
    
  using W::on;
  
  std::function<popup_menu(event_context&)> popup_opener;
};

} // widgets

namespace weave::views {

template <class V, class Filter, class Action>
struct event_listener : V {
  using widget_t = widgets::event_listener<typename V::widget_t>;
  
  event_listener(auto&& v, Filter filter, Action action) 
    : V{(decltype(v)&&)v}, filter{filter}, action{action} {}
  
  template <class State>
  auto build(const build_context& b, State& state) {
    return widget_t{V::build(b, state), [action = action, filter = filter] (event_context& ec, input_event e) {
      auto mapped_event = std::invoke(filter, e);
      if (!mapped_event)
        return false;
      context_invoke<State>(action, ec, *mapped_event);
      return true;
    }};
  }
  
  auto rebuild(auto& old, widget_ref r, auto&& ctx, auto& state) {
    return V::rebuild(old, widget_ref{&r.as<widget_t>().base()}, ctx, state);
  }
  
  void destroy(widget_ref r, application_context& ctx) {
    V::destroy(widget_ref(&r.as<widget_t>().base()), ctx);
  }
  
  Filter filter;
  Action action;
};

template <class V, class Filter, class Fn>
event_listener(V, Filter filter, Fn fn) -> event_listener<V, Filter, Fn>;

inline bool is_mouse_down(input_event e) {
  return e.is_mouse() && e.mouse().is_down();
}

template <class V>
struct with_fixed_size : V {
  
  using widget_t = widgets::with_fixed_size<typename V::widget_t>; 
  
  auto build(const build_context& ctx, auto& state) {
    auto res = widget_t{ V::build(ctx, state) };
    res.set_size(fixed_size);
    return res;
  }
  
  rebuild_result rebuild(const with_fixed_size<V>& Old, widget_ref r, const build_context& ctx, auto& state) {
    auto& wb = r.as<widget_t>();
    V::rebuild(Old, widget_ref{&static_cast<typename V::widget_t&>(wb)}, ctx, state);
    if (fixed_size != Old.fixed_size) { 
      wb.set_size(fixed_size);
      return rebuild_result::size_change;
    }
    return {};
  }
   
  point fixed_size;
};

template <class V>
struct with_nominal_size : V {
  
  using widget_t = widgets::with_nominal_size<typename V::widget_t>; 
  
  auto build(const build_context& ctx, auto& state) {
    return widget_t{ V::build(ctx, state), nominal_size };
  }
  
  rebuild_result rebuild(const with_nominal_size<V>& Old, widget_ref r, const build_context& ctx, auto& state) {
    auto& wb = r.as<widget_t>();
    V::rebuild((V&)Old, widget_ref{&static_cast<typename V::widget_t&>(wb)}, ctx, state);
    if (nominal_size != Old.nominal_size) { 
      wb.nominal_size = nominal_size;
      return rebuild_result::size_change;
    }
    return {};
  }
  
  void destroy(widget_ref w, application_context& ctx) {
    V::destroy(widget_ref(&(typename V::widget_t&)w.as<widget_t>()), ctx);
  }
   
  point nominal_size;
};

template <class V, class Fn>
struct animated : V {
  
  using widget_t = typename V::widget_t;
  
  rebuild_result rebuild(const animated<V, Fn>& Old, widget_ref r, const build_context& ctx, auto& state) {
    auto& w = r.as<widget_t>();
    auto& app_ctx = ctx.application_context();
    if (flag != Old.flag) {
      if (flag)
        app_ctx.animate(w, fn, period_ms);
      else
        app_ctx.deanimate(r);
    }
    return V::rebuild(Old, r, ctx, state);
  }
  
  void destroy(widget_ref r, application_context& ctx) {
    if (flag)
      ctx.deanimate(r);
    V::destroy(widget_ref((typename V::widget_t*)&r.as<widget_t>()), ctx);
  }
  
  bool flag; 
  Fn fn;
  int period_ms;
};

template <class V, class B>
  requires (is_view<V> && is_view<B>)
struct with_background;

template <class V>
struct with_flex_factor : V {
  
  using widget_t = widgets::with_flex_factor<typename V::widget_t>;
  
  auto build(const build_context& ctx, auto& state) {
    return widget_t{ V::build(ctx, state), flex_factor };
  }
  
  rebuild_result rebuild(const with_flex_factor<V>& Old, widget_ref r, const build_context& ctx, auto& state) {
    auto& wb = r.as<widget_t>();
    V::rebuild((V&)Old, widget_ref{&static_cast<typename V::widget_t&>(wb)}, ctx, state);
    if (flex_factor != Old.flex_factor) { 
      wb.flex_factor = flex_factor;
      return rebuild_result::size_change;
    }
    return {};
  }
  
  void destroy(widget_ref r, application_context& ctx) {
    V::destroy(widget_ref((typename V::widget_t*)&r.as<widget_t>()), ctx);
  }
  
  point flex_factor;
};

template <class V, class Fn>
struct with_popup_menu : V {
  
  using widget_t = widgets::with_popup_menu<typename V::widget_t>;
  
  auto build(const build_context& ctx, auto& state) {
    auto res = widget_t{ V::build(ctx, state) };
    res.popup_opener = opener;
    return res;
  }
  
  rebuild_result rebuild(const with_popup_menu<V, Fn>& Old, widget_ref r, const build_context& ctx, auto& state) {
    auto& wb = r.as<widget_t>();
    V::rebuild((V&)Old, widget_ref{&static_cast<typename V::widget_t&>(wb)}, ctx, state);
    wb.popup_opener = opener;
    return {};
  }
  
  void destroy(widget_ref r, application_context& ctx) {
    V::destroy(widget_ref((typename V::widget_t*)&r.as<widget_t>()), ctx);
  }
  
  Fn opener;
};

/// Common extensions for views.
struct view_modifiers {

  private : 
  
  struct one_keydown_filter {
    std::optional<keyboard_event> operator()(input_event e) const { 
      if (e.is_keyboard() && e.keyboard().is_down() && e.keyboard().key() == key)
        return e.keyboard();
      return {};
    }
    keycode key;
  };
  
  static std::optional<keyboard_event> keydown_filter(input_event e) {
    if (e.is_keyboard() && e.keyboard().is_down())
      return e.keyboard();
    return {};
  }
  
  static std::optional<std::string> file_drop_filter(input_event e) {
    if (e.is_mouse() && e.mouse().is_file_drop())
      return e.mouse().dropped_file();
    return {};
  }
  
  static std::optional<mouse_event> click_filter(input_event e) {
    if (e.is_mouse() && e.mouse().is_down())
      return e.mouse();
    return {};
  }
  
  public : 
  
  auto on_click(this auto&& self, auto action) {
    return event_listener{WEAVE_FWD(self), &click_filter, std::move(action)};
  }
  
  auto on_key_down(this auto&& self, auto action) {
    return event_listener{WEAVE_FWD(self), &keydown_filter, std::move(action)};
  }
  
  auto on_key_down(this auto&& self, keycode k, auto action) {
    return event_listener{WEAVE_FWD(self), one_keydown_filter{k}, std::move(action)};
  }
  
  auto on_file_drop(this auto&& self, auto action) {
    return event_listener{WEAVE_FWD(self), &file_drop_filter, std::move(action)};
  }
  
  auto with_fixed_size(this auto&& self, point size) {
    return views::with_fixed_size{WEAVE_FWD(self), size};
  }
  
  auto with_nominal_size(this auto&& self, point size) {
    return views::with_nominal_size{WEAVE_FWD(self), size};
  }
  
  auto animate_when(this auto&& self, bool flag, int period_ms, auto fn) {
    return animated{WEAVE_FWD(self), flag, fn, period_ms};
  }
  
  auto background(this auto&& self, auto&& background) {
    return with_background{WEAVE_FWD(self), WEAVE_FWD(background)};
  }
  
  auto with_flex_factor(this auto&& self, point flex_factor) {
    return views::with_flex_factor{WEAVE_FWD(self), flex_factor};
  }
  
  auto with_popup_menu(this auto&& self, auto&& opener) {
    return views::with_popup_menu{WEAVE_FWD(self), WEAVE_FWD(opener)};
  }
};

template <class V, class B>
  requires (is_view<V> && is_view<B>)
struct with_background : V {
  
  using widget_t = widgets::with_background<typename V::widget_t, typename B::widget_t>;

  auto build(const build_context& ctx, auto& state) {
    return widgets::with_background{V::build(ctx, state), background.build(ctx, state)};
  }
  
  rebuild_result rebuild (const with_background<V, B>& Old, widget_ref w, 
                          const build_context& ctx, auto& state) {
    using foreground = decltype(V::build(ctx, state));
    auto res = V::rebuild((V&)Old, widget_ref(&(foreground&)w.as<widget_t>()), ctx, state);
    res |=  background.rebuild(Old.background, widget_ref{&w.as<widget_t>().background}, ctx, state);
    return res;
  }
  
  void destroy(widget_ref r, application_context& ctx) {
    V::destroy(widget_ref((typename V::widget_t*)&r.as<widget_t>()), ctx);
    background.destroy(widget_ref(&r.as<widget_t>().background), ctx);
  }
  
  B background;
};

} // views