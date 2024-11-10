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
  
    void set_focused(widget& w, vec2f abs_pos) {
      set_focused(w.id(), abs_pos);
    }
  
    void set_focused(widget_id id, vec2f absolute_pos) {
      if (id == focused)
        return;
      if (id == widget_id{view_id{0}})
        assert( (absolute_pos == vec2f{0, 0}) && "root is focused but offset is not 0" );
      focused = id;
      focused_absolute_pos = absolute_pos;
    }
  
    bool find_in(widget& w, vec2f abs_pos, widget_tree& t, mouse_event e)
    {
      if (contains(w, e.position - abs_pos))
      {
        for (auto& c : t.children(w))
          if (find_in(c, abs_pos + c.pos(), t, e))
            return true;
        set_focused(w, abs_pos);
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
      return find_from(t.parent(w), abs_pos - w.pos(), t, e);
    }
  
    public : 
  
    void on(mouse_event e, widget_tree& t, void* state) 
    {
      auto w = t.find(focused);
      if (not w) {
        set_focused(widget_id{view_id{0}}, {0, 0});
        w = t.root();
      }
      if (e.is<mouse_move>() && !e.get_as<mouse_move>().is_dragging) 
      { 
        find_from(*t.find(focused), focused_absolute_pos, t, e);
        w = t.find(focused);
      }
      e.position -= focused_absolute_pos;
      w->on(e, state);
    }
  
    widget_id focused;
    vec2f focused_absolute_pos = {0, 0};
  };
  
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
  }

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
    unsigned x = 0;
    impl::init_widget_id(app_view, x);
    win.init("spiral", 600, 400);
    gctx.init();
    
    widget_tree_builder builder {tree};
    builder.construct_view(app_view);
    tree.layout();
    tree.debug_dump();
  }
  
  void run(State& state)
  {
    app_view = view_ctor(state);
    
    while(!backend.want_quit)
    {
      backend.visit_event( [this, &state] (auto&& e) {
        med.on(e, tree, &state);
      });
      paint(state);
    }
  }
  
  void paint(State& state)
  {
    painter p {gctx.ctx};
    p.begin_frame(win.size(), 1);
    
    auto impl = [this, &p, &state] (auto&& self, widget& w) -> void
    {
      auto offset = w.pos();
      p.translate(offset);
      w.paint(p, &state);
      for (auto& w : tree.children(w))
        self(self, w);
      p.translate(-offset);
    };
    
    impl(impl, tree.get(view_id{0}));
    p.end_frame();
    win.swap_buffer();
  }
};

// Construct an application with a given State and view constructor.
template <class State, class Ctor>
auto make_app(State state, Ctor view_ctor) {
  using ViewT = decltype(view_ctor(state));
  return application<Ctor, ViewT, State>{state, view_ctor};
}
