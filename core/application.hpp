#pragma once

#include <cassert>
#include <ranges>
#include <optional>
#include <atomic>
#include <chrono>

#include "backend.hpp"
#include "view.hpp"
#include "window.hpp"
#include "lens.hpp"
#include "widget.hpp"

#include "../graphics/graphics.hpp"
#include "../events/mouse_events.hpp"
#include "../util/vec.hpp"
#include "../util/util.hpp"

#include "nfd.h"

namespace weave {

namespace impl {

  struct mouse_event_dispatcher 
  {
    private : 
  
    struct find_new_focused 
    {
      mouse_event_dispatcher& self;
      mouse_event e;
      event_context_parent_stack& parents;
      
      bool find_in_children(widget_ref w, vec2f abs_pos) {
        auto c = w.find_child_at(e.position - abs_pos);
        if (!c)
          return false;
        parents.push_back(w);
        if (find_in_children(*c, abs_pos + c->position())) 
          return true;
        self.set_focused(*c);
        return true;
      }
      
      bool find_from(widget_ref w, vec2f abs_pos) 
      {
        // Note : widget::contains assumes a point relative to the position 
        // of the *parent*, and abs_pos contains the position of w, so we have 
        // to add the position of w 
        if (w.contains(e.position - abs_pos + w.position())) {
          if (find_in_children(w, abs_pos)) {
            return true;
          }
          self.set_focused(w);
          return true;
       }
       else {
        if (parents.empty()) {
          // It's important that we call set_focused here as the event may lay outside 
          // of the root widget, but we still want to make it focused if no other widget can
          // (otherwise we'll end up focusing a child widget with an empty parent stack)
          self.set_focused(w);
          return true;
        }
        auto p = parents.back(); 
        parents.pop_back();
        return find_from(p, abs_pos - w.position());
       }
      }
    };
    
    void set_focused(widget_ref w) {
      set_focused(w, parents);
    }
    
    public : 
    
    mouse_event_dispatcher(widget_ref root) {
      set_focused(root, {});
    }
    
    void set_focused(widget_ref w, const event_context_parent_stack& new_parents) {
      if (w == focused)
        return;
      
      // it will often be the case that &parents == &new_parents
      if (&parents != &new_parents)
        parents = new_parents;
      
      // Note : In a lot of cases we don't have to recompute the position entirely from the root
      // but it's good to recompute it once in a while
      vec2f absolute_pos = {0, 0};
      for (auto p : parents)
        absolute_pos += p.position();
      absolute_pos += w.position();
      
      focused = w;
      focused_absolute_pos = absolute_pos;
      
      top_parent_listener = {};
      for (auto p : std::ranges::views::reverse(parents))
        if (p.is_child_event_listener())
          top_parent_listener = p;
    }
    
    void reset_focus(widget_ref root) {
      parents.clear();
      set_focused(root, {});
    }
    
    event_frame_result on(mouse_event e, void* state, application_context& ctx)
    {
      event_frame_result res;
      
      if (e.is_move() && !e.is_drag())
      {
        auto old_focus = focused;
        auto old_pos = focused_absolute_pos;
        auto old_ctx = event_context{ctx, res, parents, state};
        find_new_focused{*this, e, parents}.find_from(focused, focused_absolute_pos);
        if (old_focus != focused) 
        {
          old_focus.on(mouse_event{e.position - old_pos, mouse_exit{}}, old_ctx);
          auto ec = event_context{ctx, res, parents, state};
          focused.on(mouse_event{e.position - focused_absolute_pos, mouse_enter{}}, ec);
        }
      }
      
      // FIXME : avoid the copy of the parents vector here
      auto ec = event_context{ctx, res, parents, state};
      e.position -= focused_absolute_pos;
      focused.on(e, ec);
      
      if (top_parent_listener) {
        e.position += focused.position();
        for (auto p : std::ranges::views::reverse(parents)) 
        {
          p.on_child_event(e, ec, focused);
          e.position += p.position();
          if (top_parent_listener == p)
            break;
        }
      }
      
      return res;
    }
    
    vec2f focused_absolute_position() const {
      return focused_absolute_pos;
    }
    
    widget_ref current_focus() const { return focused; }
    
    void update_absolute_position() {
      focused_absolute_pos = {0, 0};
      for (auto& p : parents) 
        focused_absolute_pos += p.position();
      focused_absolute_pos += focused.position();
    }
    
    private : 
    
    widget_ref focused;
    vec2f focused_absolute_pos = {0, 0};
    event_context_parent_stack parents;
    optional<widget_ref> top_parent_listener = {};
  };
  
  struct keyboard_event_dispatcher {
    
    void set_focused(widget_ref r, const event_context_parent_stack& stack) {
      if (focused == r)
        return;
      focused = r;
      parents = stack;
    }
    
    void reset_focus() {
      focused.reset();
      parents.clear();
    }
    
    event_frame_result on(keyboard_event e, void* state, application_context& ctx) {
      event_frame_result res;
      if (focused) {
        auto ec = event_context{ctx, res, parents, state};
        focused->on(e, ec);
      }
      return res;
    }
    
    optional<widget_ref> focused;
    event_context_parent_stack parents;
  };
  
  struct widget_animations {
    
    struct animation {
      
      widget_ref widget;
      std::function<bool(widget_ref)> call;
      int period_in_ms;
      std::chrono::steady_clock::time_point last_call;
    };
    
    private :
    
    std::vector<animation> animations;
    
    public :
    
    template <class W, class Fn>
    void animate(Widget& widget, Fn fn, int period_ms)
    {
      animations.push_back(
          animation{
            &widget,
            [fn] (widget_ref w) -> bool { return fn(w.as<Widget>()); },
            period_ms,
            std::chrono::steady_clock::now()
          }
      );
    }
    
    void deanimate(widget_ref w) {
      erase_if(animations, [widget] (auto& a) { return a.widget == widget;} );
    }
  
    /// Return true if a repaint is needed 
    bool run()
    {
      if (animations.size() == 0)
        return false;
      
      using namespace std::chrono;
      const auto now = steady_clock::now();
      
      bool gotta_repaint = false;
      
      erase_if( animations, [now, &gotta_repaint] (auto& a)
      {
        const auto time_since_last = int(duration_cast<milliseconds>(now - a.last_call).count());
        
        if (time_since_last < a.period_in_ms)
          return false;
        
        gotta_repaint = true;
        a.last_call = now;
        return not a.exec();
      });
      return gotta_repaint;
    }
  };
} // impl

template <class ViewCtor, class View, class State>
struct application;

std::optional<std::basic_string<nfdchar_t>> open_file_dialog() 
{
  NFD_Init();
  nfdchar_t* outPath;
  nfdresult_t result = NFD_OpenDialog(&outPath, nullptr, 0, nullptr);
  if (result == NFD_OKAY) {
    std::string res{outPath};
    NFD_FreePath(outPath);
    NFD_Quit();
    return res;
  }
  NFD_Quit();
  return {};
}

std::optional<std::basic_string<nfdchar_t>> save_file_dialog()
{
  NFD_Init();
  nfdchar_t* outPath;
  nfdresult_t result = NFD_SaveDialog(&outPath, nullptr, 0, nullptr, nullptr);
  if (result == NFD_OKAY) {
    std::string res {outPath};
    NFD_FreePath(outPath);
    NFD_Quit();
    return res;
  }
  NFD_Quit();
  return {};
}

struct application_context {
  
  template <class ViewCtor, class View, class State>
  friend struct application;
  
  template <class RootCtor>
  application_context(window_properties win_prop, RootCtor Ctor)
  : backend{this},
    win{win_prop}, 
    root{Ctor()}, 
    overlay{nullptr},
    med{root.borrow()}
  {
    layout_root();
    auto size_info = root.size_info();
    win.set_min_size(size_info.min);
    win.set_max_size(size_info.max);
  }
  
  template <class W, class Fn>
  void animate(W& widget, Fn fn, int period_in_ms) {
    animations.animate(widget, fn, period_in_ms);
  }
  
  void deanimate(widget_ref r) {
    animations.deanimate(r);
  }
  
  void grab_mouse_focus(widget_ref new_focused, const event_context_parent_stack& parents) {
    med.set_focused(new_focused, parents);
  }
  
  widget_ref push_overlay(widget_box w) 
  {
    overlay = std::move(w);
    med.set_focused(overlay.borrow(), {});
    return overlay.borrow();
  }
  
  void pop_overlay()
  {
    med.set_focused(root.borrow(), {});
    overlay.reset();
  }
  
  void grab_keyboard_focus(widget_ref r, const event_context_parent_stack& stack) {
    keyboard.set_focused(r, stack);
  }
  
  void release_keyboard_focus() {
    keyboard.reset_focus();
  }
  
  widget_ref current_mouse_focus() {
    return med.current_focus();
  }
  
  bool has_keyboard_focus(widget_ref r) const {
    return keyboard.focused == r; 
  }
  
  void reset_mouse_focus() {
    med.reset_focus(root.borrow());
  }
  
  bool is_active(key_modifier mod) const {
    return backend.is_active(mod);
  }
  
  graphics_context& graphics_context() {
    return gctx;
  }
  
  auto& window() {
    return win;
  }
  
  void request_rebuild() {
    rebuild_requested = true;
  }
  
  /// Implementation only.
  void paint() {
    painter p = graphics_context().painter();
    p.set_font("default");
    p.begin_frame(win.size(), 1);
    
    auto fn = [this, &p] (this auto&& self, widget_ref w, vec2f scissor_pos, vec2f scissor_sz) -> void
    {
      auto pos = w.position();
      auto sz = w.size();
      
      auto new_scissor_pos = max(scissor_pos, pos);
      new_scissor_pos = min(scissor_pos + scissor_sz, new_scissor_pos);
      
      auto new_scissor_end = min(scissor_pos + scissor_sz, pos + sz);
      auto new_scissor_sz = new_scissor_end - new_scissor_pos;
      new_scissor_sz = max(new_scissor_sz, {0, 0});
      
      // p.stroke_style(colors::red);
      // p.stroke_rect(new_scissor_pos, new_scissor_sz);
      //p.scissor(new_scissor_pos, new_scissor_sz);
      auto traii = p.translate(pos);
      w.paint(p);
      for (auto& w : w.children())
        self(w, new_scissor_pos - pos, new_scissor_sz);
    };
    
    fn(root.borrow(), {0, 0}, win.size());
    
    if (overlay) {
      fn(overlay.borrow(), {0, 0}, overlay.size());
    }
    
    p.end_frame();
    win.swap_buffer();
  }
  
  void on_window_resize() {
    debug_log("window resize");
    layout_root();
    paint();
  }
  
  std::atomic<bool> rebuild_requested = false;
  
  private : 
  
  void layout_root() {
    std::cerr << "\n window size " << win.size() << std::endl;
    root.layout(win.size());
    root.debug_dump(3);
    auto size_info = root.size_info();
    win.set_min_size(size_info.min);
    win.set_max_size(size_info.max);
  }
  
  impl::sdl_backend backend;
  struct window win;
  struct graphics_context gctx;
  widget_box root, overlay;
  impl::mouse_event_dispatcher med;
  impl::keyboard_event_dispatcher keyboard;
  widget_animations animations;
};

auto event_context::lift_rebuild_request() {
  return [p = &context()] {
    p->rebuild_requested = true;
  };
}

void event_context::push_overlay(widget_box widget) {
  ctx.push_overlay(std::move(widget));
  request_repaint();
}

void event_context::pop_overlay() {
  ctx.pop_overlay();
  request_repaint();
}

void event_context::grab_mouse_focus(widget_ref focused) {
  // Truncate the parents stack if this is a parent
  auto it = std::find(parents.begin(), parents.end(), focused);
  if (it != parents.end())
    ctx.grab_mouse_focus(focused, event_context_parent_stack{parents.begin(), it});
  else
    ctx.grab_mouse_focus(focused, parents);
}

void event_context::grab_keyboard_focus(widget_ref focused) {
  ctx.grab_keyboard_focus(focused, parents);
}

void event_context::release_keyboard_focus() {
  ctx.release_keyboard_focus();
}

widget_ref event_context::current_mouse_focus() {
  return ctx.current_mouse_focus();
}

template <class W, class Fn>
void event_context::animate(W& widget, Fn fn, int period_in_ms) {
  ctx.animate(widget, fn, period_in_ms);
}
  
void event_context::deanimate(widget_ref w) {
  ctx.deanimate(w);
}

void event_context::reset_mouse_focus() {
  ctx.reset_mouse_focus();
}

void event_context::push_overlay_relative(widget_box widget) {
  auto abs_pos = absolute_position() + widget.position();
  auto winsz = ctx.window().size();
  auto new_pos = abs_pos;
  new_pos -= max(new_pos + widget.size() - winsz, point{0, 0});
  widget.set_position(new_pos);
  push_overlay(std::move(widget));
}

template <class ViewCtor, class View, class State>
struct application 
{
  ViewCtor view_ctor;
  /// Some views are not assignable, so we emplace the main view instead
  std::optional<View> app_view;
  application_context app_ctx;
  
  application(State& s, ViewCtor ctor, window_properties prop) 
  : view_ctor{ctor}, 
    app_view{view_ctor(s)},
    app_ctx{ prop, 
          [&, this] { return app_view->build(build_context{impl}, s); } }
  {
    app_ctx.paint();
    // impl.root.debug_dump(3);
  }
  
  void rebuild(State& state) {
    debug_log("rebuilding");
    auto old_view = std::move(*app_view);
    app_view.emplace( view_ctor(state) );
    auto bctx = build_context{impl};
    app_view->rebuild(old_view, impl.root.borrow(), bctx, state);
    app_ctx.med.update_absolute_position();
    app_ctx.rebuild_requested = false;
    // Note : it would be nice to not have to do this on every rebuild ? 
    auto size_info = impl.root.size_info();
    app_ctx.win.set_min_size(size_info.min);
    app_ctx.win.set_max_size(size_info.max);
  }
  
  void run(State& state)
  {
    while(!app_ctx.backend.want_quit)
    {
      event_frame_result frame;
      app_ctx.backend.visit_event( [&, this] (auto&& e) {
        if constexpr ( std::is_same_v<std::remove_reference_t<decltype(e)>, keyboard_event> )
          frame = impl.keyboard.on(e, &state, impl);
        else
          frame = impl.med.on(e, &state, impl);
      });
      
      if (frame.rebuild_requested || impl.rebuild_requested) {
        rebuild(state);
        frame.repaint_requested = true;
      }
      
      if (frame.repaint_requested)
        app_ctx.paint(); 
    }
  }
};

/// Construct an application with a given State and view constructor.
template <class State, class Ctor>
auto make_app(State& state, Ctor view_ctor, window_properties prop) {
  using ViewT = decltype(view_ctor(state));
  return application<Ctor, ViewT, State>{state, view_ctor, std::move(prop)};
}

} // weave
