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

struct animation_context {
  
  template <class T>
  T& state() { return *static_cast<T*>(state_ptr); }
  
  void* state_ptr;
};

namespace impl {

  struct mouse_event_dispatcher 
  {
    private : 
  
    struct find_new_focused 
    {
      mouse_event_dispatcher& self;
      mouse_event e;
      const widget_tree& tree;
      
      bool find_in_children(widget_ref w, point abs_pos) {
        auto c = w.find_child_at(e.position - abs_pos);
        if (!c)
          return false;
        if (find_in_children(*c, abs_pos + c->position())) 
          return true;
        self.set_focused(c->id(), tree);
        return true;
      }
      
      bool find_from(widget_ref w, point abs_pos) 
      {
        // Note : widget::contains assumes a point relative to the position 
        // of the *parent*, and abs_pos contains the position of w, so we have 
        // to add the position of w 
        if (w.contains(e.position - abs_pos + w.position())) {
          if (find_in_children(w, abs_pos)) {
            return true;
          }
          self.set_focused(w.id(), tree);
          return true;
       }
       else {
        auto pid = tree.parent_of(w.id());
        if (pid == w.id()) {
          self.set_focused(w.id(), tree);
          return true;
        }
        auto p = *tree.get(pid); 
        return find_from(p, abs_pos - w.position());
       }
      }
    };
    
    public : 
    
    mouse_event_dispatcher(widget_id root) : focused{root} {}
    
    void set_focused(widget_id id, const widget_tree& tree) {
      if (id == focused)
        return;
      
      focused = id;
      
      // Note : In a lot of cases we don't have to recompute the position entirely from the root
      // but it's good to recompute it once in a while
      update_absolute_position(tree);
      
      top_parent_listener = {};
      for (auto p : tree.parents_of(id)) {
        if (p.is_child_event_listener())
          top_parent_listener = p.id();
      }
    }
    
    event_result on(mouse_event e, void* state, application_context& ctx);
    
    point focused_absolute_position() const {
      return focused_absolute_pos;
    }
    
    widget_id current_focus() const { return focused; }
    
    widget_ref current_focus(const widget_tree& tree, widget_id root_id) {
      auto ref = tree.get(focused);
      if (ref)
        return *ref;
      focused = root_id;
      auto res = tree.get(focused);
      focused_absolute_pos = res->position();
      return *res;
    }
    
    void update_absolute_position_after_rebuild(const widget_tree& tree, widget_id root_id) {
      auto ref = tree.get(focused);
      if (!ref)
        focused = root_id;
      update_absolute_position(tree);
    }
    
    void update_absolute_position(const widget_tree& tree) {
      optional<widget_ref> f = tree.get(focused);
      assert( f && "focused not found in the tree?" );
      focused_absolute_pos = f->absolute_position(tree);
    }
    
    private : 
    
    widget_id focused;
    point focused_absolute_pos = {0, 0};
    optional<widget_id> top_parent_listener;
  };
  
  struct keyboard_event_dispatcher {
    
    void set_focused(widget_id id) {
      focused = id;
    }
    
    event_result reset_focus(application_context& ctx, void* state);
    
    event_result on(keyboard_event e, void* state, application_context& ctx);
    
    optional<widget_id> focused;
  };
  
  struct widget_animations {
    
    struct animation {
      
      // Return false if the animation should stop
      bool run(std::chrono::steady_clock::time_point now, void* state_ptr) {
        last_call = now;
        return call(widget, state_ptr);
      }
      
      widget_ref widget;
      std::function<bool(widget_ref, void*)> call;
      int period_in_ms;
      std::chrono::steady_clock::time_point last_call;
    };
    
    private :
    
    std::vector<animation> animations;
    
    public :
    
    template <class Widget, class Fn>
    void animate(Widget& widget, Fn fn, int period_ms)
    {
      animations.push_back(
          animation{
            &widget,
            [fn] (widget_ref w, void* state) -> bool { 
              auto ctx = animation_context{state};
              return fn(w.as<Widget>(), ctx); 
            },
            period_ms,
            std::chrono::steady_clock::now()
          }
      );
    }
    
    void deanimate(widget_ref w) {
      erase_if(animations, [w] (auto& a) { return a.widget == w;} );
    }
  
    /// Return true if a repaint is needed 
    bool run(void* state_ptr)
    {
      if (animations.size() == 0)
        return false;
      
      using namespace std::chrono;
      const auto now = steady_clock::now();
      
      bool gotta_repaint = false;
      
      erase_if( animations, [now, &gotta_repaint, state_ptr] (auto& a)
      {
        const auto time_since_last = int(duration_cast<milliseconds>(now - a.last_call).count());
        
        if (time_since_last < a.period_in_ms)
          return false;
        
        gotta_repaint = true;
        
        return not a.run(now, state_ptr);
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
    mouse{root.id()}
  {
    backend.start_text_input(win);
    root.mount(tree, root.id());
    layout_root();
  }
  
  widget_ref root_widget() const {
    return root.borrow();
  }
  
  template <class W, class Fn>
  void animate(W& widget, Fn fn, int period_in_ms) {
    animations.animate(widget, fn, period_in_ms);
  }
  
  void deanimate(widget_ref r) {
    animations.deanimate(r);
  }
  
  void grab_mouse_focus(widget_id new_focused) {
    mouse.set_focused(new_focused, widget_tree());
  }
  
  widget_ref push_overlay(widget_box w) 
  {
    overlays.push_back(std::move(w));
    auto& o = overlays.back();
    o.mount(widget_tree(), o.id());
    mouse.set_focused(o.id(), widget_tree());
    return o.borrow();
  }
  
  void pop_overlay(widget_id id)
  {
    // find the root widget
    id = widget_tree().last_parent_of(id).id();
    auto it = std::ranges::find_if(overlays, [id] (auto& e) { return e.id() == id; });
    if (it == overlays.end())
      return;
    widget_tree().erase(id);
    it->destroy(destroy_context{widget_tree()});
    overlays.erase(it);
  }
  
  void grab_keyboard_focus(widget_id id) {
    keyboard.set_focused(id);
  }
  
  event_result reset_keyboard_focus(void* state) {
    return keyboard.reset_focus(*this, state);
  }
  
  widget_ref current_mouse_focus() {
    return mouse.current_focus(widget_tree(), root.id()); 
  }
  
  optional<widget_id> current_keyboard_focus() const {
    return keyboard.focused;
  }
  
  bool has_keyboard_focus(widget_ref r) const {
    return keyboard.focused == r.id(); 
  }
  
  bool has_mouse_focus(widget_ref r) const {
    return mouse.current_focus() == r.id();
  }
  
  void reset_mouse_focus() {
    mouse.set_focused(root.id(), widget_tree());
  }
  
  bool is_held(key_modifier mod) const {
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
    
    auto fn = [this, &p] (this auto&& self, widget_ref w, point scissor_pos, point scissor_sz) -> void
    {
      auto pos = w.position();
      auto sz = w.size();
      
      // FIXME : this is a rectangle intersection!
      auto new_scissor_pos = max(scissor_pos, pos);
      new_scissor_pos = min(scissor_pos + scissor_sz, new_scissor_pos);
      
      auto new_scissor_end = min(scissor_pos + scissor_sz, pos + sz);
      auto new_scissor_sz = new_scissor_end - new_scissor_pos;
      new_scissor_sz = max(new_scissor_sz, {0, 0});
      
      // p.stroke_style(colors::red);
      // p.stroke_rect(new_scissor_pos, new_scissor_sz);
      auto scissor_raii = p.scissor(new_scissor_pos, new_scissor_sz);
      auto traii = p.translate(pos);
      w.paint(p);
      for (auto& w : w.children())
        self(w, new_scissor_pos - pos, new_scissor_sz);
    };
    
    fn(root_widget(), {0, 0}, win.size());
    
    for (auto& o : overlays)
      fn(o.borrow(), {0, 0}, win.size());
    
    /*
    auto f = current_mouse_focus();
    auto r = rectangle(f.absolute_position(tree), f.size());
    p.fill_style(rgba_f{colors::red}.with_alpha(0.3));
    p.fill(r);*/
    
    p.end_frame();
    win.swap_buffer();
  }
  
  void on_window_resize() {
    debug_log("window resize");
    layout_root();
    paint();
  }
  
  struct widget_tree& widget_tree() {
    return tree;
  }
  
  const struct widget_tree& widget_tree() const {
    return tree;
  }
  
  std::atomic<bool> rebuild_requested = false;
  
  private : 
  
  void layout_root() {
    root.layout(win.size());
    root.debug_dump();
    auto size_info = root_widget().size_info();
    win.set_min_size(size_info.min);
    win.set_max_size(size_info.max);
  }
  
  impl::sdl_backend backend;
  struct window win;
  struct graphics_context gctx;
  
  struct widget_tree tree;
  widget_box root;
  std::vector<widget_box> overlays;
  impl::mouse_event_dispatcher mouse;
  impl::keyboard_event_dispatcher keyboard;
  impl::widget_animations animations;
};

namespace impl {

event_result mouse_event_dispatcher::on(mouse_event e, void* state, 
                                        application_context& ctx)
{
  event_result res;
  auto ec = event_context{ctx, res, state};
  
  if (e.is_move() && !e.is_drag())
  {
    auto old_focus = ctx.widget_tree().get(focused);
    if (old_focus)
    {
      auto old_pos = focused_absolute_pos;
      find_new_focused{*this, e, ctx.widget_tree()}.find_from(*old_focus, focused_absolute_pos);
      if (old_focus->id() != focused)
      {
        old_focus->on(mouse_event{e.position - old_pos, mouse_exit{}}, ec);
        auto focused_ref = *ctx.widget_tree().get(focused);
        focused_ref.on(mouse_event{e.position - focused_absolute_pos, mouse_enter{}}, ec);
      }
    }
  }
  
  // Release the keyboard focus on a click 
  if (e.is_down() && focused != ctx.current_keyboard_focus())
    res = res || ctx.reset_keyboard_focus(state);
  
  // focused::on might mutate focused 
  const auto focused_id = focused;
  const auto top_parent_listener_id = top_parent_listener;
  
  e.position -= focused_absolute_pos;
  auto focused_ref = ctx.widget_tree().get(focused_id);
  assert( focused_ref && "focused not found in the tree" );
  focused_ref->on(e, ec);
  
  // focused::on might have relocated or deleted focused, look it up again
  focused_ref = ctx.widget_tree().get(focused_id);
  // We might have a new focused_id
  if (!focused_ref) {
    auto new_focus_id = focused_id != focused ? focused : ctx.root_widget().id();
    set_focused(new_focus_id, ctx.widget_tree());
    return res;
  }
  
  if (top_parent_listener_id) {
    e.position += focused_ref->position();
    for (auto p : ctx.widget_tree().parents_of(*focused_ref))
    {
      p.on_child_event(e, ec, *focused_ref);
      e.position += p.position();
      if (top_parent_listener_id == p.id())
        break;
    }
  }
  
  return res;
}

event_result keyboard_event_dispatcher::reset_focus(application_context& ctx, void* state) {
  if (!focused)
    return {};
  auto wr = ctx.widget_tree().get(*focused);
  event_result res;
  if (wr) {
    auto ec = event_context{ctx, res, state};
    wr->on(input_event{keyboard_focus_release{}}, ec);
  }
  focused.reset();
  return res;
}

event_result keyboard_event_dispatcher::on(keyboard_event e, void* state, 
                                           application_context& ctx) {
  event_result res;
  if (focused) { 
    auto wb = ctx.widget_tree().get(*focused);
    auto ec = event_context{ctx, res, state};
    // Reset the focus if focused was deleted
    if (!wb)
      focused.reset();
    else
      wb->on(e, ec);
  }
  return res;
}
    
} // impl

auto event_context::lift_rebuild_request() {
  return [p = &context()] {
    p->rebuild_requested = true;
  };
}

void event_context::push_overlay(widget_box widget) {
  ctx.push_overlay(std::move(widget));
  request_repaint();
}

void event_context::pop_overlay(widget_id id) {
  ctx.pop_overlay(id);
  request_repaint();
}

void event_context::grab_mouse_focus(widget_id focused) {
  ctx.grab_mouse_focus(focused);
}

void event_context::grab_keyboard_focus(widget_id focused) {
  ctx.grab_keyboard_focus(focused);
}

void event_context::release_keyboard_focus() {
  ctx.reset_keyboard_focus(state_ptr);
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
  auto abs_pos = widget.absolute_position(tree());
  auto winsz = ctx.window().size();
  auto new_pos = abs_pos;
  new_pos -= max(new_pos + widget.size() - winsz, point{0, 0});
  widget.set_position(new_pos);
  push_overlay(std::move(widget));
}

graphics_context& event_context::graphics_context() const {
  return ctx.graphics_context();
}

bool event_context::is_held(key_modifier mod) const {
  return ctx.is_held(mod);
}

widget_ref event_context::parent_of(widget_base& b) const {
  return tree().parent_ref(b.id());
}

widget_tree& event_context::tree() const {
  return ctx.widget_tree();
}

graphics_context& build_context::graphics_context() const { 
  return ctx.graphics_context(); 
}

widget_tree& build_context::widget_tree() const {
  return ctx.widget_tree();
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
          [&, this] { return app_view->build(build_context{app_ctx}, s); } }
  {
    app_ctx.paint();
  }
  
  void rebuild(State& state) {
    debug_log("rebuilding");
    auto old_view = std::move(*app_view);
    app_view.emplace( view_ctor(state) );
    auto bctx = build_context{app_ctx};
    app_view->rebuild(old_view, app_ctx.root_widget(), bctx, state);
    app_ctx.mouse.update_absolute_position_after_rebuild(app_ctx.widget_tree(), 
                                                        app_ctx.root_widget().id());
    app_ctx.rebuild_requested = false;
    // FIXME : it would be nice to not have to do this on every rebuild ? 
    auto size_info = app_ctx.root_widget().size_info();
    app_ctx.win.set_min_size(size_info.min);
    app_ctx.win.set_max_size(size_info.max);
  }
  
  void run(State& state)
  {
    while(!app_ctx.backend.want_quit)
    {
      event_result frame;
      app_ctx.backend.visit_event( [&, this] (auto&& e) {
        if constexpr ( std::is_same_v<std::remove_reference_t<decltype(e)>, keyboard_event> )
          frame = app_ctx.keyboard.on(e, &state, app_ctx);
        else
          frame = app_ctx.mouse.on(e, &state, app_ctx);
      });
      
      if (frame.rebuild_requested || app_ctx.rebuild_requested) {
        rebuild(state);
        frame.repaint_requested = true;
      }
      
      frame.repaint_requested = frame.repaint_requested || app_ctx.animations.run(&state);
      
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
