#include <iostream>
#include <SDL.h>
#include <glad/glad.h>
#include <concepts>
#include <cassert>
#include <iostream>

#include "spiral.hpp"

#include <iostream>

template <class T>
auto& operator<<(std::ostream& os, const vec<T, 2>& v) {
  os << "{" << v.x << " " << v.y << "}";
  return os;
}

auto& operator<<(std::ostream& os, widget_id id) {
  os << id.base.value << "." << id.offset;
  return os;
}

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
			//fprintf(stderr, "failed to initialize the OpenGL context.");
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
        auto ev = mouse_event{pos(e.button.x, e.button.y), mouse_down{to_mouse_button(e.button.button), false}};
        vis(ev); 
        break;
      }
      
      case SDL_MOUSEBUTTONUP :
      {
        mouse_is_dragging = false;
        auto p = pos(e.button.x, e.button.y);
        vis( mouse_event{p, mouse_up{}} );
        break;
      }
      
      case SDL_MOUSEMOTION :
      {
        auto p = pos(e.motion.x, e.motion.y);
        vis( mouse_event{p, mouse_move{}} );
        break;
      }
      
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

template <class... Ts>
struct stack : composed_view {
  template <class Fn>
  void traverse(Fn fn) {
    apply([fn] (auto&&... args) {
      (fn(args), ...);
    }, children);
  }
  
  tuple<Ts...> children;
};

struct vstack_widget : layout_tag 
{
  void paint(painter& p, vec2f) {}
  void on(input_event) {}
  
  auto layout( widget_tree::children_view c ) 
  {
    float pos = 0, width = 0;
    for (auto& n : c) {
      n.set_pos(0, pos);
      pos += n.size().y;
      width = std::max(width, n.size().x);
    }
    
    return vec2f{width, pos};
  }
};

template <class... Ts>
struct vstack : stack<Ts...>
{ 
  vstack(Ts... ts) : stack<Ts...>{{}, {ts...}} {}
  
  template <class Ctx>
  auto construct(Ctx ctx) {
    return ctx.template create_widget<vstack_widget>(this->view_id, vec2f{0, 0}); 
    // auto new_ctx = ctx.template construct_stateless<vstack_widget>(this->view_id);
  }
};

struct mouse_event_dispatcher 
{
  static bool contains(widget_tree::widget& w, vec2f p) {
    auto pos = w.pos();
    auto sz = w.size();
    return (p.x >= pos.x && p.x <= pos.x + sz.x && p.y >= pos.y && p.y <= pos.y + sz.y);
  }
  
  void set_focused(widget_id id, vec2f offset) {
    if (id == focused)
      return;
    std::cout << "set focused " << id << " " << offset << std::endl;
    focused = id;
    current_offset = offset;
  }
  
  // offset + widget.pos = absolute position
  bool try_push_down(mouse_event e, widget_tree& t, widget_id id, vec2f offset = {0, 0}) 
  { 
    assert( offset.x >= 0 && offset.y >= 0 && "negative offset?" );
    std::cout << "try push down " << e.position << " " << id << " " << offset << std::endl;
    auto w = t.find(id);
    assert( w );
    auto pos = w->pos();
    auto sz = w->size();
    
    if (contains(*w, e.position - offset))
    {
      for (auto& cid : w->children())
        if (try_push_down(e, t, cid, offset + pos))
          return true;
      set_focused(id, offset + pos);
      return true;
    }
    return false;
  }
  
  void try_push_up(mouse_event e, widget_tree& t, widget_id id, vec2f offset = {0, 0})
  { 
    assert( offset.x >= 0 && offset.y >= 0 && "negative offset?" );
    std::cout << "try push up " << e.position << " " << id << " " << offset << std::endl;
    
    auto w = t.find(id);
    if (contains(*w, e.position - offset)) {
      set_focused(id, offset);
    }
    else {
      auto next = w->parent_id();
      if (next != id)
        try_push_up(e, t, next, offset - w->pos());
    }
  }
  
  void on(mouse_event e, widget_tree& t, void* state) 
  {
    auto w = t.find(focused);
    if (not w) {
      focused = widget_id{view_id{0}};
      current_offset = {0, 0};
    }
    if (e.is<mouse_move>()) 
    { 
      if (try_push_down(e, t, focused, current_offset))
        return;
      else 
        try_push_up(e, t, focused, current_offset);
    }
    else
      w->on(e, state);
  }
  
  widget_id focused;
  vec2f current_offset = {0, 0};
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

struct slider_x_node 
{
  using value_type = float;
  
  struct widget_action_context {
    
  };
  
  void on(mouse_event e, vec2f this_size, event_context<float> ec) {
    if (!e.is<mouse_down>())
      return;
    auto sz = this_size;
    float ratio = e.position.x / sz.x;
    ec.write(ratio);
    //ec.repaint_request();
  }
  
  void paint(painter& p, float pc, vec2f this_size) {
    auto sz = this_size;
    p.fill_style(rgba_f{1.f, 0.f, 0.f, 1.f});
    p.rectangle({0, 0}, sz);
    p.fill_style(colors::lime);
    auto e = pc;
    p.rectangle({0, 0}, {e * sz.x, sz.y});
  }
};

template <class Lens>
struct slider : view {
  
  slider(Lens) {}
  
  auto construct(widget_tree_builder& b) {
    return b.create_widget<slider_x_node, Lens>( view::view_id, size ); //size);
  }
  
  vec2f size = {80, 15};
};

template <class Lens>
slider(Lens) -> slider<Lens>;

template <class T, field_decl FD>
struct lifted_field_lens
{
  constexpr decltype(auto) operator()(auto&& head) {
    return head.%(FD);
  }
  
  using value_type = %type_of(FD);
  
  value_type& read(void* state) const { return static_cast<T*>(state)->%(FD); }
  
  void write(value_type val, void* state) const { 
    read(state) = val;
  }
};

consteval expr lens(field_expr f) {
  auto ty = remove_reference(type_of(child(f, 0)));
  return ^ [f, ty] (lifted_field_lens< %typename(ty), field(f) >{});
}

#define LENS(EXPR) simple_lens<decltype(EXPR), [] (auto& state) -> auto&& { return EXPR; }>

template <class View>
void construct_tree(widget_tree& tree, View view) {
  widget_tree_builder b {tree};
  view.construct(b);
}

template <class ViewCtor, class View, class State>
struct application 
{
  sdl_backend backend;
  window win;
  
  ViewCtor view_ctor;
  View app_view;
  
  graphics_context gctx;
  
  mouse_event_dispatcher med;
  
  widget_tree tree;
  
  application(State& s, ViewCtor ctor) : view_ctor{ctor}, app_view{view_ctor(s)}
  {
    unsigned x = 0;
    init_widget_id(app_view, x);
    win.init("spiral", 600, 400);
    gctx.init();
    
    widget_tree_builder builder {tree};
    builder.construct_view(app_view);
    tree.layout();
  }
  
  void run(State& state)
  {
    app_view = view_ctor(state);
    
    while(!backend.want_quit)
    {
      backend.visit_event( [this, &state] (auto&& e) {
        med.on(e, tree, &state);
      });
      
      //if (not process_event(app, backend))
      // break;
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

template <class State, class Ctor>
auto make_app(State state, Ctor view_ctor) {
  using ViewT = decltype(view_ctor(state));
  return application<Ctor, ViewT, State>{state, view_ctor};
}
  
struct AppState {
  float x = 0.3, y = 0.5;
};

auto make_demo_app(AppState& state)
{
  return vstack{ slider{%lens(^(state.x))}, slider{%lens(^(state.y))} };
}

int main()
{
  AppState state;
  auto app = make_app(state, &make_demo_app);
  app.run(state);
}