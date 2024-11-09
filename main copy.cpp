#include <iostream>
#include <SDL.h>
#include <glad/glad.h>
#include <concepts>

#include "spiral.hpp"

class sdl_backend
{
  bool mouse_is_dragging = false;
  
  public :
  
  bool want_quit = false;
  
  sdl_backend()
  {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
      fprintf(stderr, "failed to initialize SDL, aborting.");
      exit(1);
    }
  
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  
    // setting up a stencil buffer is necessary for nanovg to work
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );
  }
  
  void load_opengl()
  {
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
		{
			fprintf(stderr, "failed to initialize the OpenGL context.");
			exit(1);
		}
  }
  
  template <class Fn>
  void visit_event(Fn vis) 
  {
    SDL_Event e;
    if (not SDL_WaitEventTimeout(&e, 15))
      return;
    
    switch(e.type)
    {
      case SDL_MOUSEBUTTONDOWN :
      {
        auto ev = mouse_down{pos(e.button.x, e.button.y), to_mouse_button(e.button.button), false};
        vis(ev); 
        break;
      }
      
      case SDL_MOUSEBUTTONUP :
      {
        mouse_is_dragging = false;
        auto p = pos(e.button.x, e.button.y);
        vis( mouse_up{p} );
        break;
      }
      
      case SDL_MOUSEMOTION :
      case SDL_MOUSEWHEEL :
        break;
    
      case SDL_TEXTEDITING :
      case SDL_TEXTINPUT :
      case SDL_KEYDOWN :
      case SDL_KEYUP :
        break;
    
      case SDL_WINDOWEVENT :
        break;
    
      case SDL_QUIT :
        want_quit = true;
        break;
      
      default :
        break;
    }
  }
  
  ~sdl_backend()
  {
    SDL_Quit();
  }
  
  private : 
  
  static vec2f pos(int x, int y){
		return vec2f{ (float)x, (float)y };
	}
	
	static mouse_button to_mouse_button(int button) {
	  switch(button)
	  {
	    case SDL_BUTTON_LEFT : return mouse_button::left;
	    case SDL_BUTTON_MIDDLE : return mouse_button::middle;
	    case SDL_BUTTON_RIGHT : 
	      default : 
	      return mouse_button::right;
	  }
	}
};

struct window {

  window() : win{nullptr}, gl_ctx{nullptr} {}
  
  window(window&& w) noexcept {
    win    = std::exchange(w.win,  nullptr);
    gl_ctx = std::exchange(gl_ctx, nullptr);
  }
  
  window(const char* name, int x, int y)
  {
    init(name, x, y);
  }
  
  ~window(){
    if (not win) return;
    SDL_DestroyWindow(win);
    SDL_GL_DeleteContext(gl_ctx);
  }
  
  void init(const char* name, int x, int y) {
    win    = SDL_CreateWindow(name, 0, 0, x, y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    gl_ctx = SDL_GL_CreateContext(win);
  }
  
  SDL_Window* get(){ return win; }
  
  void maximize()        { SDL_MaximizeWindow(win);     }
  void minimize()        { SDL_MinimizeWindow(win);     }
  void restore()         { SDL_RestoreWindow(win);      }
  void hide()            { SDL_HideWindow(win);         }
  void show()            { SDL_ShowWindow(win);         }
  void rise()            { SDL_RaiseWindow(win);        }
  
  unsigned id() const    { return SDL_GetWindowID(win); }
  
  bool is_active() const { return win; }

  vec2f size() const {
    vec2i res;
    SDL_GetWindowSize(win, &res.x, &res.y);
    return {(float)res.x, (float)res.y};
  }
  
  vec2f position() const { return {0, 0}; }
  
  /* 
  void min_size(screen_pt sz) {
    SDL_SetWindowMinimumSize(win, sz.x, sz.y);
  }
  
  void max_size(screen_pt sz) {
    SDL_SetWindowMaximumSize(win, sz.x, sz.y);
  } */ 
  
  void swap_buffer() {
    SDL_GL_SwapWindow(win);
  }
  
  private :
  
  SDL_Window* win;
  void* gl_ctx;
};

struct widget_context_info {
  type_list emitted;
};

namespace impl {
  
  consteval type make_emit_fn(type emitted) {
    auto cd = (class_decl) emitted;
    auto res = (alias_type_decl) lookup(cd, "result");
    return make_pointer(make_function(res, type_list{^void*, emitted}));
  }
  
  consteval type widget_context_fn_table(type_list emitted) {
    template_arguments args;
    for (auto e : emitted)
      push_back(args, impl::make_emit_fn(e));
    return instantiate(^tuple, args);
  }
  
  consteval int find_first(type_list tl, type t) {
    int k = 0;
    for (auto e : tl)
    {
      if (e == t)
        return k;
      ++k;
    }
    return -1;
  }
}

namespace impl {

  template <class Context, class Signal>
  typename Signal::result vtable_emit_impl(void* ctx, Signal signal) 
  {
    return emit( *reinterpret_cast<Context*>(ctx), signal );
  }

  consteval expr_list signals_vtable_inits(class_decl Table, type Context) 
  {
    expr_list res;
    for (auto f : fields(Table))
    {
      auto fnt = (function_type) pointee( (pointer_type) type_of(f) );
      template_arguments args {Context, parameters(fnt)[1]};
      auto fn_inst = instantiate(^vtable_emit_impl, args);
      auto e = make_operator_expr(operator_kind::address_of, make_decl_ref_expr(fn_inst));
      push_back(res, e);
    }
    return res;
  }
  
  template <class Table, class Context>
  static constexpr Table signals_vtable = %(expr)signals_vtable_inits(^Table, ^Context);
}

template <type_list emitted>
struct widget_context 
{
  using table = %impl::widget_context_fn_table(emitted); 
  
  template <class Ev>
  static constexpr auto index_of = impl::find_first(emitted, ^Ev);
  
  template <class T>
    requires (is_instance_of(^T, ^ui_tree_path))
  widget_context(T ctx)
  {
    static_assert( sizeof(T) == sizeof(void*) );
    std::construct_at<T>( reinterpret_cast<T*>(&value), ctx);
    table_ptr = &impl::signals_vtable<table, T>;
  }
  
  template <class Ev>
    requires (index_of<Ev> != -1)
  decltype(auto) emit(Ev e) {
    return get<index_of<Ev>>(*table_ptr)( (void*) &value, e);
  }
  
  private : 
  
  const table* table_ptr;
  void* value; 
};

template <class App>
struct application_context {
  typename App::state state;
};

struct poop {};

struct Child
{
  rgb_u8 col;
  
  using context = widget_context<{(type)^state_write<float>, (type)^state_read<float>}>;
  
  void on(mouse_down e, context ctx)
  {
    //emit(ctx, state_write<float>{e.position.x});
    ctx.emit( state_write<float>{e.position.x} );
    std::cout << "child " << ctx.emit(state_read<float>{}) << std::endl;
    //emit(ctx, poop{});
  }
  
  auto size() const { return vec2f{30, 30}; }
  
  void paint(painter& p){
    p.fill_style(col);
    p.rectangle({0, 0}, size());
  }
};

struct slider
{
  using context = widget_context<{(type)^state_write<float>, (type)^state_read<float>}>;
  
  vec2f size() const {
    return {100, 20};
  }
  
  void on(mouse_down e, context ctx) {
    value_ratio = e.position.x / size().x;
    ctx.emit(state_write<float>{value_ratio});
    std::cout << value_ratio << std::endl;
  }
  
  void paint(painter& p) {
    p.fill_style(colors::white);
    float sizex = size().x;
    p.rectangle(vec2f{0, 0}, vec2f{sizex, 20});
    p.fill_style(colors::green);
    std::cout << value_ratio * sizex << std::endl;
    p.rectangle(vec2f{0, 0}, vec2f{value_ratio * sizex, 20});
  }
  
  float value_ratio = 0;
};

struct button 
{
  
  vec2f size() const {
    return {20, 20};
  }
  
  
};

consteval void declare_children(class_builder& b, expr e)
{
  auto f = append_field(b, "child_layout", type_of(e), e);
  
  method_prototype mp;
  return_type(mp, make_lvalue_reference(type_of(e)));
  append_method( b, "layout", mp, [f] (method_builder& b) {
    append_return(b, make_field_expr(b, f));
  }); 
}

struct MyApp
{
  float x = 0;
  float y = 0;
  
  %declare_children
  ( 
    ^( col(mutator<^x>(slider{}), mutator<^y>(slider{})) )
  );
  
  void on(mouse_down e, ignore)
  {
    std::cout << "bro" << std::endl;
  }
  
  void receive(poop, ignore) {
    std::cout << "received poop" << std::endl;
  }

  void paint(painter& p) 
  {
    //p.fill_style(rgba_f{0.4f, 1.f, 1.f, 1.f});
    //p.rectangle({0, 0}, {30, 60});
  }
};

template <class EventReceiver, class Event, class Context>
decltype(auto) dispatch_event(Event ev, EventReceiver& receiver, Context context)
{
  if constexpr (requires {receiver.on(ev, context);})
    return receiver.on(ev, context);
}

template <class Widget, class Fn>
bool traverse_children(Widget& w, Fn fn)
{
  if constexpr ( requires {w.layout();} )
  {
    return w.layout().traverse(fn);
  }
  return true;
}

template <class Widget>
void paint(window& window, Widget& w, graphics_context& ctx)
{
  auto painter = ctx.painter();
  painter.begin_frame(window.size(), 1);
  w.paint(painter);
  traverse_children( w, [&painter] (auto& c, vec2f position, ignore) 
  {
    painter.set_origin(position);
    c.paint(painter);
    return true;
  });
  painter.end_frame();
  window.swap_buffer();
}

/// the event has a position relative to w
template <class Context, class Widget>
void process_mouse_down(Context context, Widget& w, mouse_down ev)
{
  auto impl = [context, ev] (auto& child, vec2f position, auto index) mutable 
  {
    ev.position -= position;
    process_mouse_down( push_back(context, index), child, ev ); 
  };
  
  if constexpr ( requires {w.layout();} )
  {
    if ( w.layout().visit_at(ev.position, impl) )
      return;
  }
  
  dispatch_event(ev, w, context);
}

template <class App, class EventInput>
bool process_event(App& app, EventInput& ei)
{
  ei.visit_event( [&app] (auto ev) 
  {
    if constexpr (type_of(^ev) == ^mouse_down)
    {
      process_mouse_down(ui_tree_path<App>{{}, &app}, app, ev);
    }
    else
    {
      dispatch_event(ev, app, ui_tree_path{});
    }
    return true;
  });
  return not ei.want_quit;
}

namespace impl {
  
  template <class Path>
  void process_mouse_move(Path p, mouse_move ev)
  {
    
  }
}

struct mouse_event_dispatcher
{ 
  template <class T>
  using ptr = T*;
  
  template <class Ctx, class Event>
  static void vtable_slot(void* ctx, Event ev) 
  {
    auto& path = *reinterpret_cast<Ctx*>(ctx);
    path().on(ev, path);
  }
  
  template <class Path>
  static void process_mouse_move(Path p, mouse_move ev)
  {
  }
  
  static constexpr type_list mouse_events {^mouse_down, ^mouse_up, ^mouse_drag, ^mouse_move};
  
  struct vtable 
  {
    % [] (class_builder& b) 
    {
      int k = 0;
      for (auto t : mouse_events)
      {
        auto t = make_pointer(make_function(^void, {^void*, t}));
        append_field(b, std::meta::cat("p", k++), t);
      }
    } ();
  };
  
  expr_list gen_vtable_impl(type path) 
  {
    expr_list res;
    for (auto t : mouse_events)
    {
      template_arguments args {path, t};
      push_back(res, make_decl_ref_expr(instantiate(^vtable_slot, args)));
    }
    return res;
  }
  
  template <class Path>
  static constexpr auto vtable_impl = %(expr) gen_vtable_impl(^Path);
  
  template <class P>
    requires is_instance_of(^ui_tree_path, ^P)
  void set_focus(P path)
  {
    std::construct_at<P>( reinterpret_cast<P*>(&focused), ctx );
    vptr = &vtable_impl<P>;
  }
  
  void* focused;
  const vtable* vptr;
  vec2f focus_pt;
};

template <class App>
struct application 
{
  sdl_backend backend;
  window win;
  App app;
  graphics_context gctx;
  
  application()
  {
    win.init("spiral", 600, 400);
  }
  
  void run()
  {
    while( true )
    {
      if (not process_event(app, backend))
        break;
      paint(win, app, gctx);
    }
  }
};

int main()
{
  Application<MyApp> app;
  app.run();
}