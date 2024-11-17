#pragma once

#include <cassert>

#include "backend.hpp"
#include "ui_tree.hpp"
#include "layout.hpp"
#include "view.hpp"
#include "window.hpp"
#include "lens.hpp"

#include "../graphics/graphics.hpp"
#include "../events/mouse_events.hpp"
#include "../vec.hpp"

namespace impl {

  struct mouse_event_dispatcher 
  {
    private : 
  
    static bool contains(widget& w, vec2f p) {
      auto sz = w.size();
      return (p.x >= 0 && (p.x <= sz.x) && p.y >= 0 && p.y <= sz.y);
    }
  
    void set_focused(widget& w, vec2f abs_pos, widget_tree& tree) {
      set_focused(w.id(), abs_pos, tree);
    }
  
    void set_focused(widget_id id, vec2f absolute_pos, widget_tree& tree) {
      if (id == focused)
        return;
      if (id == widget_id::root())
        assert( (absolute_pos == vec2f{0, 0}) && "root is focused but offset is not 0" );
      focused = id;
      focused_absolute_pos = absolute_pos;
      
      parent_listeners.clear();
      auto parent = tree.parent_id(id);
      while (parent != widget_id::root())
      {
        auto& w = tree.get(parent);
        if (w.is_child_event_listener())
          parent_listeners.push_back(parent);
        parent = tree.parent_id(parent);
      }
    }
    
    bool find_in_children(widget& w, vec2f abs_pos, widget_tree& t, mouse_event e)
    {
      auto c = w.find_child_at(e.position - abs_pos);
      if (!c)
        return false;
      if (find_in_children(*c, abs_pos + c->position(), t, e))
        return true;
      set_focused(*c, abs_pos + c->position(), t);
      return true;
    }
    
    bool find_in(widget& w, vec2f abs_pos, widget_tree& t, mouse_event e)
    {
      if (w.contains(e.position - abs_pos))
      {
        if (find_in_children(w, abs_pos, t, e))
          return true;
        set_focused(w, abs_pos, t);
        return true;
      }
      return false;
    }
  
    bool find_from(widget& w, vec2f abs_pos, widget_tree& t, mouse_event e)
    {
      if (find_in(w, abs_pos, t, e))
        return true;
      if (w.id() == w.parent_id())
        return true;
      return find_from(t.parent(w), abs_pos - w.position(), t, e);
    }
  
    public : 
  
    void on(mouse_event e, widget_tree& t, void* state) 
    {
      auto w = t.find(focused);
      if (not w) {
        set_focused(widget_id::root(), {0, 0}, t);
        w = &t.root();
      }
      
      auto ecd = event_context_data{t, state};
      
      if (e.is_mouse_move() && !e.is_mouse_drag()) 
      {
        auto old_focus = focused;
        auto old_pos = focused_absolute_pos;
        find_from(*t.find(focused), focused_absolute_pos, t, e);
        w = t.find(focused);
        if (old_focus != focused) 
        {
          t.find(old_focus)->on(mouse_event{e.position - old_pos, mouse_exit{}}, ecd);
          w->on(mouse_event{e.position - focused_absolute_pos, mouse_enter{}}, ecd);
        }
      }
      
      e.position -= focused_absolute_pos;
      w->on(e, ecd);
      for (auto p : parent_listeners)
        t.get(p).on_child_event(e, ecd, focused);
    }
    
    void layout_changed(widget_tree& t) {
      vec2f new_pos = {0, 0};
      auto id = focused;
      while(id != widget_id::root())
      {
        auto& w = t.get(id);
        new_pos += w.position();
        id = w.parent_id();
      }
      focused_absolute_pos = new_pos;
    }
    
    widget_id focused = widget_id::root();
    vec2f focused_absolute_pos = {0, 0};
    std::vector<widget_id> parent_listeners;
  };
  
  /* 
  template <class View>
  void init_widget_id(View& v, unsigned& base) 
  {
    v.view_id = view_id{base++};
  
    if constexpr (is_base_of(^composed_view, ^View))
    {
      v.traverse( [&base] (auto& e)
      {
        init_widget_id(e, base);
      });
    }
  } */ 

} // impl

template <class ViewCtor, class View, class State>
struct application 
{
  sdl_backend backend;
  window win;
  
  ViewCtor view_ctor;
  View app_view;
  
  graphics_context gctx;
  
  impl::mouse_event_dispatcher med;
  
  widget_tree tree;
  
  application(State& s, ViewCtor ctor) : view_ctor{ctor}, app_view{view_ctor(s)}
  {
    //unsigned x = 0;
    //impl::init_widget_id(app_view, x);
    win.init("spiral", 600, 400);
    gctx.init();
    
    auto b = tree.builder();
    auto [w, lens, args] = app_view.build(b, s);
    auto root = tree.create_widget(args.id, std::move(w), lens, args);
    assert( root == &tree.root() );
    tree.root().layout();
  }
  
  void run(State& state)
  {
    while(!backend.want_quit)
    {
      backend.visit_event( [this, &state] (auto&& e) {
        med.on(e, tree, &state);
      });
      
      auto new_view = view_ctor(state);
      auto upd = tree.updater();
      app_view.rebuild(new_view, tree.root(), upd, state);
      med.layout_changed(tree);
      
      paint(state);
    }
  }
  
  void paint(State& state)
  {
    painter p {gctx.ctx};
    p.set_font("default");
    p.begin_frame(win.size(), 1);
    
    auto impl = [this, &p, &state] (auto&& self, widget& w, vec2f scissor_pos, vec2f scissor_sz) -> void
    {
      auto pos = w.position();
      auto sz = w.size();
      
      auto new_scissor_pos = max(scissor_pos, pos);
      new_scissor_pos = min(scissor_pos + scissor_sz, new_scissor_pos);
      
      auto new_scissor_end = min(scissor_pos + scissor_sz, pos + sz);
      auto new_scissor_sz = new_scissor_end - new_scissor_pos;
      new_scissor_sz = max(new_scissor_sz, {0, 0});
      
      //p.scissor( scissor_pos, scissor_sz );
      
      // p.stroke_style(colors::red);
      // p.stroke_rect(new_scissor_pos, new_scissor_sz);
      p.scissor(new_scissor_pos, new_scissor_sz);
      p.translate(pos);
      w.paint(p, &state);
      for (auto& w : tree.children(w))
        self(self, w, new_scissor_pos - pos, new_scissor_sz);
      //p.pop_transform();
      p.translate(-pos);
    };
    
    impl(impl, tree.root(), {0, 0}, win.size());
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
