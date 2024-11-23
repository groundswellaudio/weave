#include "spiral.hpp"

struct SpringSim : app_state {
  
  struct Particle {
    float force = 0.f, speed = 0.f, pos = 0.f, mass = 1.f;
  };
  
  struct Relation {
    float force, damp;
    int ia, ib;
  };
  
  auto& get_particle(int index) {
    return particles[index];
  }
  
  auto& get_relation(int index) {
    return rels[index];
  }
    
  void add_particle() {
    auto _ = write_scope();
    particles.push_back({0, 0, 1.f});
  }
  
  void on_connection(int ia, int ib) {
    auto _ = write_scope();
    rels.push_back(Relation{1.f, 0.f, ia, ib});
  }
  
  void render_audio(audio_output_stream os)
  {
    auto _ = read_scope();
    
    for (auto s : os) 
    {
      for (auto r : rels) {
        auto delta = particles[r.ia].pos - particles[r.ib].pos;
        particles[r.ia].force -= delta * r.force;
        particles[r.ib].force += delta * r.force;
      }
    }
  }
  
  std::vector<Particle> particles;
  std::vector<Relation> rels;
};

struct SpringSimApp : SpringSim {
  
  struct Selection {
    int index = -1;
    int kind = 0;
  };

  Selection selection;
};

struct Editor : widget_base {
  
  using value_type = SpringSimApp&;
  
  struct Particle : widget_base
  {
    using value_type = void;
    
    void on(mouse_event e, event_context<Particle>& ec) 
    {
    }
     
    void paint(painter& p) {
      p.fill_style(colors::red);
      p.circle(size() / 2, size().x / 2);
      if (selected) {
        p.stroke_style(colors::white);
        p.stroke_circle(size() / 2, size().x / 2, 2);
      }
    }
    
    bool selected = false;
  };
  
  void on_child_event(mouse_event e, event_context<Editor>& ctx, widget_ref child) {
    if (e.is_mouse_down()) {
      ctx.read().selection.index = &child.as<Particle>() - particles.data();
    }
    else if (e.is_mouse_drag()) {
      mouse_pos = e.position;
    }
    else if (e.is_mouse_up()) {
      auto w = find_child_at(e.position);
      if (!w)
        return;
      auto found = &w->as<Particle>() - particles.data();
      auto selected_idx = ctx.read().selection.index;
      if (found != selected_idx)
        create_connection(selected_idx, found, ctx);
    }
  }
  
  //Editor(Properties prop) : prop{prop} {}
  
  vec2f layout() const {
    return {400, 400};
  }
  
  bool traverse_children(auto&& fn) {
    return std::all_of(particles.begin(), particles.end(), fn);
  }
  
  void create_connection(int IndexA, int IndexB, event_context<Editor>& ec) {
    connections.push_back({IndexA, IndexB}); 
    ec.read().on_connection(IndexA, IndexB);
  }
  
  void on(mouse_event e, event_context<Editor>& ec) {
    if (!e.is_mouse_down())
      return;
    if (e.is_double_click()) {
      particles.push_back( Particle{{{20, 20}, e.position}} );
      ec.read().add_particle();
    }
    else {
      int idx = 0;
      traverse_connections( [&ec, &idx, &e] (vec2f a, vec2f b) {
        if (distance((a + b) / 2, e.position) < 5) {
          auto& s = ec.read().selection;
          s.index = idx;
          s.kind = 1;
          return false;
        }
        ++idx;
        return true;
      });
    }
  }
  
  void paint(painter& p, ignore) 
  {
    p.fill_style(colors::blue);
    p.rectangle({0, 0}, size());
    p.stroke_style(colors::green);
    //p.circle(mouse_pos)
    auto center = [] (auto& w) { return w.position() + w.size() / 2; };
    
    if (connecting) {
      p.line(center(*connecting), mouse_pos, 4);
    }
    
    p.fill_style(colors::green);
    
    traverse_connections( [&p] (vec2f a, vec2f b) {
      p.circle((a + b) / 2, 5);
      p.line(a, b, 4);
      return true;
    });
  }
  
  private : 
  
  void traverse_connections(auto fn) {
    auto center = [] (auto& w) { return w.position() + w.size() / 2; };
    for (auto [a, b] : connections) {
      auto CenterA = center(particles[a]);
      auto CenterB = center(particles[b]);
      if (!fn(CenterA, CenterB))
        return;
    }
  }
  
  std::vector<Particle> particles;
  std::vector<tuple<int, int>> connections;
  Particle* connecting = nullptr;
  vec2f mouse_pos;
};

//using EditorWL = with_lens<Editor>
// static_assert( is_child_event_listener<with_lens<Editor, dyn_lens<SpringSimApp&>>> );

template <class T>
struct view_with_body : T {
  
  decltype(std::declval<T>().body()) body_v;
  
  view_with_body(auto&&... args) : T{args...}, body_v{T::body()} {} 
  
  auto build(auto&& builder, auto& state) {
    return body_v.build(builder, state);
  }
  
  void rebuild(auto& New, widget_ref w, auto&& updater, auto& state) {
    auto NewBody = New.body_v;
    body_v.rebuild(NewBody, w, updater, state);
  }
};

static constexpr auto EditorLens = [] (auto& s) -> auto& { return s; };

using EditorView = simple_view_for<Editor, decltype(EditorLens)>;

//using MainView = view_with_body<MainViewT>;

auto make_spring_sim(SpringSimApp& state)
{ 
  auto selec_index = state.selection.index;
  auto RightPanel = maybe {
    selec_index != -1, 
    either {
      (unsigned) state.selection.kind,
      slider{ [selec_index] (auto& s) -> auto& { return s.get_particle(selec_index).mass; } }
      .with_range(0.01, 100),
      slider{ [selec_index] (auto& s) -> auto& { return s.get_relation(selec_index).force; } }
      .with_range(1, 100)
    }
  };
  
  return hstack {
    EditorView {
      EditorLens
    },     
    //slider { MLens(x0.particles.back().mass) }
    RightPanel
  };
}

inline void run_spring_sim() {
  SpringSimApp state;
  auto app = make_app(state, &make_spring_sim);
  app.run(state);
}