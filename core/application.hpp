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
        self.set_focused(*c, abs_pos + c->position());
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
          self.set_focused(w, abs_pos);
          return true;
       }
       else {
        if (parents.empty()) {
          // It's important that we call set_focused here as the event may lay outside 
          // of the root widget, but we still want to make it focused if no other widget can
          // (otherwise we'll end up focusing a child widget with an empty parent stack)
          self.set_focused(w, abs_pos);
          return true;
        }
        auto p = parents.back(); 
        parents.pop_back();
        return find_from(p, abs_pos - w.position());
       }
      }
    };
    
    void set_focused(widget_ref w, vec2f absolute_pos) {
      if (w == focused)
        return;
      
      if (parents.size() == 0)
        assert( (absolute_pos == vec2f{0, 0}) && "root is focused but offset is not 0" );
      
      focused = w;
      focused_absolute_pos = absolute_pos;
      
      top_parent_listener = {};
      for (auto p : std::ranges::views::reverse(parents))
        if (p.is_child_event_listener())
          top_parent_listener = p;
    }
    
    public : 
    
    mouse_event_dispatcher(widget_ref root) {
      set_focused(root, {0, 0});
    }
    
    void reset_focus(widget_ref root) {
      parents.clear();
      set_focused(root, {0, 0});
    }
    
    void on(mouse_event e, void* state, application_context& ctx)
    {
      auto ecd = event_context_data{ctx, parents, state};
      
      if (!is_modal && e.is_mouse_move() && !e.is_mouse_drag())
      {
        auto old_focus = focused;
        auto old_pos = focused_absolute_pos;
        find_new_focused{*this, e, parents}.find_from(focused, focused_absolute_pos);
        if (old_focus != focused) 
        {
          old_focus.on(mouse_event{e.position - old_pos, mouse_exit{}}, ecd);
          focused.on(mouse_event{e.position - focused_absolute_pos, mouse_enter{}}, ecd);
        }
      }
      
      e.position -= focused_absolute_pos;
      focused.on(e, ecd);
      
      if (top_parent_listener) {
        e.position += focused.position();
        for (auto p : std::ranges::views::reverse(parents)) 
        {
          p.on_child_event(e, ecd, focused);
          e.position += p.position();
          if (top_parent_listener == p)
            break;
        }
      }
    }
    
    void open_modal_menu(widget_ref r, event_context_parent_stack parents_stack) {
      vec2f new_abs_pos = r.position();
      for (auto& p : parents_stack)
        new_abs_pos += p.position();
      focused = r;
      focused_absolute_pos = new_abs_pos;
      parents = std::move(parents_stack);
      is_modal = true;
    }
    
    void close_modal_menu() {
      assert( is_modal && "close_modal_menu called outside of modal mode" );
      focused_absolute_pos -= focused.position();
      focused = parents.back();
      parents.pop_back();
      is_modal = false;
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
    
    void on(keyboard_event e, void* state, application_context& ctx) {
      if (focused) {
        auto ecd = event_context_data{ctx, parents, state};
        focused->on(e, ecd);
      }
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
  
  void open_modal_menu(widget_box menu, widget_ref parent, event_context_parent_stack stack) 
  {
    stack.push_back(parent);
    modal_menu = std::move(menu);
    med.open_modal_menu(modal_menu.borrow(), std::move(stack));
  }
  
  void close_modal_menu() 
  {
    med.close_modal_menu();
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
  
  void reset_mouse_focus() {
    med.reset_focus(root.borrow());
  }
  
  bool is_active(key_modifier mod) const {
    return backend.is_active(mod);
  }
  
  ::graphics_context& graphics_context() {
    return gctx;
  }
  
  private : 
  
  sdl_backend backend;
  window win;
  struct graphics_context gctx;
  widget_box root, modal_menu;
  impl::mouse_event_dispatcher med;
  impl::keyboard_event_dispatcher keyboard;
};

template <class W, class P>
void event_context_base::open_modal_menu(this auto& self, W&& widget, P* parent) {
  self.ctx.open_modal_menu((W&&)widget, parent, self.parents);
}

void event_context_base::close_modal_menu() {
  ctx.close_modal_menu();
}

void event_context_base::grab_keyboard_focus(this auto& self, widget_ref focused) {
  self.ctx.grab_keyboard_focus(focused, self.parents);
}

void event_context_base::release_keyboard_focus() {
  ctx.release_keyboard_focus();
}

widget_ref event_context_base::current_mouse_focus() {
  return ctx.current_mouse_focus();
}
  
void event_context_base::reset_mouse_focus() {
  ctx.reset_mouse_focus();
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
  }
  
  void run(State& state)
  {
    while(!impl.backend.want_quit)
    {
      impl.backend.visit_event( [this, &state] (auto&& e) {
        if constexpr ( remove_reference(type_of(^e)) == ^keyboard_event )
          impl.keyboard.on(e, &state, impl);
        else
          impl.med.on(e, &state, impl);
      });
      
      auto old_view = *app_view;
      app_view.emplace( view_ctor(state) );
      auto upd = widget_updater{impl};
      app_view->rebuild(old_view, impl.root.borrow(), upd, state);
      
      /* 
      if (rebuild_res | rebuild_result::size_change) {
        impl.root.layout();
        impl.med.layout_changed();
      } */ 
      
      paint(state);
    }
  }
  
  void paint(State& state)
  {
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
      p.translate(pos);
      w.paint(p, &state);
      for (auto& w : w.children())
        self(w, new_scissor_pos - pos, new_scissor_sz);
      p.translate(-pos);
    };
    
    fn(impl.root.borrow(), {0, 0}, impl.win.size());
    
    if (impl.modal_menu) {
      auto abs_pos = impl.med.focused_absolute_position();
      p.translate(abs_pos);
      impl.modal_menu.paint(p, &state);
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
