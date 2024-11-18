#pragma once

#include <cassert>

#include "backend.hpp"
#include "layout.hpp"
#include "view.hpp"
#include "window.hpp"
#include "lens.hpp"
#include "widget.hpp"

#include "../graphics/graphics.hpp"
#include "../events/mouse_events.hpp"
#include "../vec.hpp"

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
        parents.push_front(w);
        if (find_in_children(*c, abs_pos + c->position())) 
          return true;
        self.set_focused(*c, abs_pos + c->position());
        return true;
      }
      
      bool find_in(widget_ref w, vec2f abs_pos) {
        if (w.contains(e.position - abs_pos)) {
          if (find_in_children(w, abs_pos)) {
            parents.push_front(w);
            return true;
          }
          self.set_focused(w, abs_pos);
          return true;
        }
        return false;
      }
      
      bool find_from(widget_ref w, vec2f abs_pos) {
        if (find_in(w, abs_pos))
          return true;
        if (parents.size() == 0)
          return true;
        auto p = parents.back();
        parents.pop_back();
        return find_from(p, abs_pos - w.position());
      }
    };
    
    void set_focused(widget_ref w, vec2f absolute_pos) {
      if (w == focused)
        return;
      if (parents.size() == 0)
        assert( (absolute_pos == vec2f{0, 0}) && "root is focused but offset is not 0" ); 
      focused = w;
      focused_absolute_pos = absolute_pos;
      
      parents_listeners.clear();
      
      for (auto p : parents)
      {
        if (p.is_child_event_listener())
          parents_listeners.push_back(p);
      }
    }
  
    public : 
    
    mouse_event_dispatcher(widget_ref root) {
      set_focused(root, {0, 0});
    }
  
    void on(mouse_event e, void* state)
    {
      auto ecd = event_context_data{parents, state};
      
      if (e.is_mouse_move() && !e.is_mouse_drag())
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
      for (auto p : parents_listeners)
        p.on_child_event(e, ecd, focused);
    }
    
    void layout_changed() {
      vec2f new_pos = focused.position();
      for (auto p : parents) 
        new_pos += p.position();
      focused_absolute_pos = new_pos;
    }
    
    widget_ref focused;
    vec2f focused_absolute_pos = {0, 0};
    event_context_parent_stack parents;
    std::vector<widget_ref> parents_listeners;
  };

} // impl

template <class ViewCtor, class View, class State>
struct application 
{
  sdl_backend backend;
  window win;
  graphics_context gctx;
  
  ViewCtor view_ctor;
  View app_view;
  
  widget_box root;
  
  impl::mouse_event_dispatcher med;
  
  application(State& s, ViewCtor ctor) 
  : win{"spiral", 600, 400},
    view_ctor{ctor}, 
    app_view{view_ctor(s)}, 
    root{app_view.build(widget_builder{}, s)}, 
    med{root.borrow()}
  {
    root.layout();
  }
  
  void run(State& state)
  {
    while(!backend.want_quit)
    {
      backend.visit_event( [this, &state] (auto&& e) {
        med.on(e, &state);
      });
      
      auto new_view = view_ctor(state);
      auto upd = widget_updater{root.borrow()};
      app_view.rebuild(new_view, root.borrow(), upd, state);
      med.layout_changed();
      
      paint(state);
    }
  }
  
  void paint(State& state)
  {
    painter p {gctx.ctx};
    p.set_font("default");
    p.begin_frame(win.size(), 1);
    
    auto impl = [this, &p, &state] (auto&& self, widget_ref w, vec2f scissor_pos, vec2f scissor_sz) -> void
    {
      auto pos = w.position();
      auto sz = w.size();
      
      auto new_scissor_pos = max(scissor_pos, pos);
      new_scissor_pos = min(scissor_pos + scissor_sz, new_scissor_pos);
      
      auto new_scissor_end = min(scissor_pos + scissor_sz, pos + sz);
      auto new_scissor_sz = new_scissor_end - new_scissor_pos;
      new_scissor_sz = max(new_scissor_sz, {0, 0});
      
      p.stroke_style(colors::red);
      p.stroke_rect(new_scissor_pos, new_scissor_sz);
      p.scissor(new_scissor_pos, new_scissor_sz);
      p.translate(pos);
      w.paint(p, &state);
      for (auto& w : w.children())
        self(self, w, new_scissor_pos - pos, new_scissor_sz);
      p.translate(-pos);
    };
    
    impl(impl, root.borrow(), {0, 0}, win.size());
    p.end_frame();
    win.swap_buffer();
  }
};

// Construct an application with a given State and view constructor.
template <class State, class Ctor>
auto make_app(State& state, Ctor view_ctor) {
  using ViewT = decltype(view_ctor(state));
  return application<Ctor, ViewT, State>{state, view_ctor};
}
