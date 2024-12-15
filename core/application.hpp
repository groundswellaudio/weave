#pragma once

#include <cassert>
#include <ranges>
#include <optional>

#include "backend.hpp"
#include "view.hpp"
#include "window.hpp"
#include "lens.hpp"
#include "widget.hpp"

#include "../graphics/graphics.hpp"
#include "../events/mouse_events.hpp"
#include "../vec.hpp"
#include "../util/util.hpp"

#include "nfd.h"

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
      
      // often the case
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
      
      if (e.is_mouse_move() && !e.is_mouse_drag())
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
    
    void layout_changed() {
      vec2f new_pos = focused.position();
      for (auto p : parents) 
        new_pos += p.position();
      focused_absolute_pos = new_pos;
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
    bool is_modal = false;
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
  application_context(const char* win_name, vec2f win_size, RootCtor Ctor)
  : win{"spiral", win_size}, 
    root{Ctor()}, 
    modal_menu{nullptr},
    med{root.borrow()}
  {
    root.layout();
  }
  
  void grab_mouse_focus(widget_ref new_focused, const event_context_parent_stack& parents) {
    med.set_focused(new_focused, parents);
  }
  
  widget_ref push_overlay(widget_box menu) 
  {
    modal_menu = std::move(menu);
    med.set_focused(modal_menu.borrow(), {});
    return modal_menu.borrow();
  }
  
  void pop_overlay()
  {
    med.set_focused(root.borrow(), {});
    modal_menu.reset();
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
  
  ::graphics_context& graphics_context() {
    return gctx;
  }
  
  auto& window() {
    return win;
  }
  
  private : 
  
  sdl_backend backend;
  struct window win;
  struct graphics_context gctx;
  widget_box root, modal_menu;
  impl::mouse_event_dispatcher med;
  impl::keyboard_event_dispatcher keyboard;
};

void event_context::push_overlay(widget_box widget) {
  ctx.push_overlay(std::move(widget));
  request_repaint();
}

void event_context::pop_overlay() {
  ctx.pop_overlay();
  request_repaint();
}

void event_context::grab_mouse_focus(widget_ref focused) {
  // Truncate the parents state if this is a parent
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

void event_context::reset_mouse_focus() {
  ctx.reset_mouse_focus();
}

void event_context::push_overlay_relative(widget_box widget) {
  auto abs_pos = absolute_position() + widget.position();
  auto winsz = ctx.window().size();
  auto new_pos = abs_pos;
  new_pos -= max(new_pos + widget.size() - winsz, vec2f{0, 0});
  widget.set_position(new_pos);
  push_overlay(std::move(widget));
}

template <class ViewCtor, class View, class State>
struct application 
{
  ViewCtor view_ctor;
  /// Some views are not assignable, so we emplace the main view instead
  std::optional<View> app_view;
  application_context impl;
  
  application(State& s, ViewCtor ctor) 
  : view_ctor{ctor}, 
    app_view{view_ctor(s)},
    impl{ "spiral", {600, 400}, 
          [&, this] { return app_view->build(widget_builder{impl}, s); } }
  {
    paint(s);
    impl.root.debug_dump(3);
  }
  
  void rebuild(State& state) {
    debug_log("rebuilding");
    auto old_view = std::move(*app_view);
    app_view.emplace( view_ctor(state) );
    auto upd = widget_updater{impl};
    app_view->rebuild(old_view, impl.root.borrow(), upd, state);
    impl.med.update_absolute_position();
  }
  
  void run(State& state)
  {
    while(!impl.backend.want_quit)
    {
      event_frame_result frame;
      impl.backend.visit_event( [&, this] (auto&& e) {
        if constexpr ( remove_reference(type_of(^e)) == ^keyboard_event )
          frame = impl.keyboard.on(e, &state, impl);
        else
          frame = impl.med.on(e, &state, impl);
      });
      
      if (frame.rebuild_requested) 
        rebuild(state);
      
      if (frame.repaint_requested)
        paint(state);
    }
  }
  
  void paint(State& state)
  {
    debug_log("painting");
    painter p = impl.graphics_context().painter();
    p.set_font("default");
    p.begin_frame(impl.win.size(), 1);
    
    auto fn = [this, &p, &state] (this auto&& self, widget_ref w, vec2f scissor_pos, vec2f scissor_sz) -> void
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
      w.paint(p, &state);
      for (auto& w : w.children())
        self(w, new_scissor_pos - pos, new_scissor_sz);
    };
    
    fn(impl.root.borrow(), {0, 0}, impl.win.size());
    
    if (impl.modal_menu) {
      fn(impl.modal_menu.borrow(), {0, 0}, impl.modal_menu.size());
    }
    
    p.end_frame();
    impl.win.swap_buffer();
  }
};

// Construct an application with a given State and view constructor.
template <class State, class Ctor>
auto make_app(State& state, Ctor view_ctor) {
  using ViewT = decltype(view_ctor(state));
  return application<Ctor, ViewT, State>{state, view_ctor};
}
