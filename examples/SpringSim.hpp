#include "spiral.hpp"

struct SpringSim : app_state {
  
};

struct Editor : widget_base {
  
  using value_type = void;
  
  struct Particle : widget_base
  {
    using value_type = void;
    
    void on(mouse_event e, event_context<Particle>& ec) 
    {
      /* 
      if (e.is_mouse_down()) {
        if (e.is_being_held(keycode::alt_left))
          ec.parent().as<Editor>().start_connect(this);
      }
      else if (e.is_mouse_drag()) {
        auto& p = ec.parent().as<Editor>();
        p.handle_particle_drag(this, e);
      }
      else if (e.is_mouse_up()) {
        
      }
      if (e.is_being_held(keycode::alt_left))
        ec.parent().as<Editor>().start_connect(this); */ 
    }
     
    void paint(painter& p) {
      p.fill_style(colors::red);
      p.circle(size() / 2, size().x / 2);
    }
  };
  
  void on_child_event(mouse_event e, event_context<Editor>& ctx, widget_ref child) {
    if (e.is_mouse_down()) {
      connecting = &child.as<Particle>();
    }
    else if (e.is_mouse_drag()) {
      //if (connecting)
      //connecting->set_position(connecting->position() + e.position);
    }
    else if (e.is_mouse_up()) {
      auto w = find_child_at(e.position);
      if (!w)
        return;
      auto found = &w->as<Particle>();
      if (found != connecting)
        create_connection(connecting, found);
    }
  }
  
  vec2f layout() const {
    return {100, 100};
  }
  
  bool traverse_children(auto&& fn) {
    return std::all_of(particles.begin(), particles.end(), fn);
  }
  
  void create_connection(Particle* a, Particle* b) {
    int IndexA = a - particles.data();
    int IndexB = b - particles.data();
    connections.push_back({IndexA, IndexB}); 
  }
  
  void on(mouse_event e, event_context<Editor>& ec) {
    if (e.is_mouse_down())
      particles.push_back( Particle{{{20, 20}, e.position}} );
  }
  
  void paint(painter& p) 
  {
    p.fill_style(colors::blue);
    p.rectangle({0, 0}, size());
    if (connecting)
      p.line(connecting->position(), current_mouse_position());
    for (auto [a, b] : connections)
      p.line(particles[a].position(), particles[b].position());
  }
  
  std::vector<Particle> particles;
  std::vector<tuple<int, int>> connections;
  Particle* connecting;
};

using EditorView = simple_view_for<Editor>;

auto make_spring_sim(SpringSim& state)
{
  return EditorView{};
};

inline void run_spring_sim() {
  SpringSim state;
  auto app = make_app(state, &make_spring_sim);
  app.run(state);
}