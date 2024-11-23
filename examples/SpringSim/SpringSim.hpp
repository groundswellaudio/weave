#include "spiral.hpp"
#include "SpringSimModel.hpp"

#include <unordered_set>

struct SpringSimApp : SpringSim {
  
  struct Selection {
    
    enum Kind {
      Empty = 0,
      Mass = 1,
      Rel = 2, 
      Mixed = 3
    };
    
    void add_particle(int id) {
      set.emplace(id);
      k = k == Rel ? Mixed : Mass;
    }
    
    void set_relation(int id) {
      rel_set.clear();
      set.clear();
      rel_set.emplace(id);
      k = Rel;
    }
    
    void set_particle(int id) {
      rel_set.clear();
      set.clear();
      set.emplace(id);
      k = Mass;
    }
    
    void clear() {
      k = Empty;
      set.clear();
      rel_set.clear();
    }
    
    int selected_particle() {
      return *set.begin();
    }
    
    int selected_relation() {
      return *rel_set.begin();
    }
    
    Kind kind() {
      return k;
    }
    
    bool empty() const { return k == Empty; }
    
    Kind k = Empty;
    std::unordered_set<int> set, rel_set;
  };
  
  auto& selected_relation() {
    return get_relation(selection.selected_relation());
  }
  
  auto& selected_particle() {
    return get_particle(selection.selected_particle());
  }
  
  auto& selected_particle_set() const {
    return selection.set;
  }
  
  Selection selection;
  int particle_kind = 0;
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
      clear_selection(ctx);
      auto& p = child.as<Particle>();
      auto index = &p - particles.data();
      p.selected = true;
      ctx.read().selection.set_particle(index);
      connecting = tuple{e.position, e.position};
    }
    else if (e.is_mouse_drag()) {
      get<1>(*connecting) = e.position;
    }
    else if (e.is_mouse_up()) {
      auto w = find_child_at(e.position);
      if (!w)
        return;
      auto found = &w->as<Particle>() - particles.data();
      auto selected_idx = ctx.read().selection.selected_particle();
      if (found != selected_idx)
        create_connection(selected_idx, found, ctx);
      connecting.reset();
    }
  }
  
  //Editor(Properties prop) : prop{prop} {}
  
  vec2f layout() const {
    return {300, 400};
  }
  
  bool traverse_children(auto&& fn) {
    return std::all_of(particles.begin(), particles.end(), fn);
  }
  
  void create_connection(int IndexA, int IndexB, event_context<Editor>& ec) {
    connections.push_back({IndexA, IndexB}); 
    ec.read().on_connection(IndexA, IndexB);
  }
  
  void update_selection_rect(vec2f pos, event_context<Editor>& ec) 
  {
    clear_selection(ec);
    int index = 0;
    selection_rect->b = pos;
    for (auto& p : particles) 
    {
      if (selection_rect->contains(p.position() + p.size() / 2)) {
        ec.read().selection.add_particle(index);
        p.selected = true;
      }
      else
        p.selected = false;
      ++index;
    }
  }
  
  void clear_selection(event_context<Editor>& ec) {
    auto& s = ec.read();
    for (auto idx : s.selected_particle_set())
      particles[idx].selected = false;
    s.selection.clear();  
  }
  
  void on(mouse_event e, event_context<Editor>& ec) 
  {
    if (e.is_mouse_drag() && selection_rect) {
      update_selection_rect(e.position, ec);
      return;
    }
    
    if (e.is_mouse_up()) {
      selection_rect.reset();
      return;
    }
    
    if (!e.is_mouse_down())
      return;
    
    if (e.is_double_click()) {
      particles.push_back( Particle{{{20, 20}, e.position}} );
      ec.read().add_particle();
      return;
    }
    
    int idx = 0;
    bool connection_found = !traverse_connections( [&ec, &idx, &e, this] (vec2f a, vec2f b) {
      if (distance((a + b) / 2, e.position) < 5) {
        clear_selection(ec);
        auto& s = ec.read().selection;
        s.set_relation(idx);
        return false;
      }
      ++idx;
      return true;
    });
    
    if (!connection_found) {
      clear_selection(ec);
      selection_rect = SelectionRect{e.position, e.position};
    }
  }
  
  void paint(painter& p, SpringSimApp& app) 
  {
    p.fill_style(colors::blue);
    p.rectangle({0, 0}, size());
    p.stroke_style(colors::green);

    if (connecting) {
      p.line(connecting->m0, connecting->m1, 4);
    }
    
    p.fill_style(colors::green);
    
    traverse_connections( [&p] (vec2f a, vec2f b) {
      p.circle((a + b) / 2, 5);
      p.line(a, b, 4);
      return true;
    });
    
    if (selection_rect) {
      p.stroke_style(colors::white);
      p.stroke_rect(selection_rect->a, selection_rect->b - selection_rect->a);
    }
  }
  
  private : 
  
  bool traverse_connections(auto fn) {
    auto center = [] (auto& w) { return w.position() + w.size() / 2; };
    for (auto [a, b] : connections) {
      auto CenterA = center(particles[a]);
      auto CenterB = center(particles[b]);
      if (!fn(CenterA, CenterB))
        return false;
    }
    return true;
  }
  
  struct SelectionRect {
    
    bool contains(vec2f pos) const {
      auto sz = abs(b - a);
      auto p = min(a, b);
      return pos.x >= p.x && pos.x <= p.x + sz.x && pos.y >= p.y && pos.y < p.y + sz.y;
    }
    
    vec2f a, b;
  };
  
  std::vector<Particle> particles;
  std::vector<tuple<int, int>> connections;
  std::optional<SelectionRect> selection_rect;
  std::optional<tuple<vec2f, vec2f>> connecting;
};

static constexpr auto EditorLens = [] (auto& s) -> auto& { return s; };

using EditorView = simple_view_for<Editor, decltype(EditorLens)>;

auto make_spring_sim(SpringSimApp& state)
{ 
  auto MassMod = 
    slider{ [] (auto& s) -> auto& { return s.get_particle(s.selection.selected_particle()).mass; } }
    .with_range(0.01, 100);
  
  auto RelForce =
    slider{ [] (auto& s) -> auto& { return s.selected_relation().force; } }
    .with_range(1, 100);
  auto RelDamp = 
    slider { [] (auto& s) -> auto& { return s.selected_relation().damp; } }
    .with_range(0, 1);
  
  auto WithLabel = [] (auto View, std::string_view str) {
    return hstack {
      text{str}, 
      View
    }.with_align(0.5);
  };
  
  auto RelMod = vstack {
    WithLabel(RelForce, "force"),
    WithLabel(RelDamp, "damp")
  };
  
  auto RightPanel = vstack { 
    combo_box_v{ [] (auto& s) -> auto& { return s.particle_kind; },
               {"Mass", "Anchor"} }, 
    either {
      (unsigned) state.selection.kind(),
      text{"No mass or particles selected"}, 
      MassMod, 
      RelMod, 
      vstack {
        MassMod, 
        RelMod
      }
    }
  //};
  }.with_margin({30, 30}).with_interspace(10);

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