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
    
    void add_rel(int idx) {
      rel_set.emplace(idx);
      k = k == Mass ? Mixed : Rel;
    }
    
    Kind kind() {
      return k;
    }
    
    bool empty() const { return k == Empty; }
    
    Kind k = Empty;
    std::unordered_set<int> set, rel_set;
  };
  
  bool selection_is_anchor() {
    for (auto m : selection.set)
      if (!get_particle(m).is_anchor())
        return false;
    return true;
  }
  
  void set_selection_mass(float val) {
    for (auto m : selection.set)
      if (!get_particle(m).is_anchor())
        get_particle(m).mass = val;
  }
  
  float get_selection_mass() {
    return get_particle(*selection.set.begin()).mass;
  }
    
  auto& selected_relation() {
    return get_relation(selection.selected_relation());
  }
  
  auto& selected_particle() {
    return get_particle(selection.selected_particle());
  }
  
  auto& selected_particle_set() const {
    return selection.set;
  }
  
  float selected_relations_force() {
    return selected_relation().force;
  }
  
  void set_selected_relations_force(float f) {
    for (auto r : selection.rel_set)
      this->get_relation(r).force = f;
  }
  
  float selected_relation_damp() {
    return selected_relation().damp;
  }
  
  void set_selected_relations_damp(float val) {
    for (auto r : selection.rel_set)
      this->get_relation(r).damp = val;
  }
  
  bool is_rel_selected(int idx) const {
    return selection.rel_set.find(idx) != selection.rel_set.end();
  }
  
  auto delete_selection() {
    return remove_particles(selection.set);
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
      p.fill_style(anchor ? colors::green : colors::red);
      p.circle(size() / 2, size().x / 2);
      if (selected) {
        p.stroke_style(colors::white);
        p.stroke_circle(size() / 2, size().x / 2, 2);
      }
    }
    
    bool selected = false;
    bool anchor = false;
  };
  
  void trigger_particle(event_context<Editor>& ctx, int index) {
    ctx.read().trigger_particle(index);
  }
  
  void on_child_event(mouse_event e, event_context<Editor>& ctx, widget_ref child) {
    if (e.is_mouse_down()) {
      auto& p = child.as<Particle>();
      auto index = &p - particles.data();
      if (e.is_double_click()) {
        trigger_particle(ctx, index);
        return;
      }
      clear_selection(ctx);
      p.selected = true;
      ctx.read().selection.set_particle(index);
      connecting = tuple{e.position, e.position};
    }
    else if (e.is_mouse_drag()) {
      get<1>(*connecting) = e.position;
    }
    else if (e.is_mouse_up()) {
      auto w = find_child_at(e.position);
      if (!w) {
        connecting.reset();
        return;
      }
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
    // Note : this is wasteful
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
    
    traverse_connections( [idx = 0, &ec, this] (vec2f a, vec2f b) mutable {
      if (selection_rect->contains((a + b) / 2))
        ec.read().selection.add_rel(idx++);
      return true;
    });
  }
  
  void clear_selection(event_context<Editor>& ec) {
    auto& s = ec.read();
    for (auto idx : s.selected_particle_set())
      particles[idx].selected = false;
    s.selection.clear();  
  }
  
  void add_particle(vec2f pos, event_context<Editor>& ec) {
    bool is_anchor = ec.read().particle_kind;
    particles.push_back( Particle{{{20, 20}, pos}, false, is_anchor} );
    ec.read().add_particle(is_anchor);
  }
  
  void on(mouse_event e, event_context<Editor>& ec) 
  {
    if (e.is_mouse_enter())
      ec.grab_keyboard_focus(this);
    
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
      add_particle(e.position, ec);
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
  
  void on(keyboard_event e, event_context<Editor>& Ec) {
    if (e.is_key_down() && e.key == keycode::backspace) {
      delete_selection(Ec);
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
    
    traverse_connections( [&p, &app, idx = 0] (vec2f a, vec2f b) mutable {
      p.circle((a + b) / 2, 5);
      p.line(a, b, 4);
      if (app.is_rel_selected(idx))
      {
        p.stroke_style(colors::white);
        p.stroke_circle((a + b) / 2, 5, 1);
        p.stroke_style(colors::green);
      }
      return true;
    });
    
    if (selection_rect) {
      p.stroke_style(colors::white);
      p.stroke_rect(selection_rect->a, selection_rect->b - selection_rect->a, 1);
    }
  }
  
  private : 
  
  void delete_selection(event_context<Editor>& Ec) {
    auto& S = Ec.read();
    // It's up to the user to signal that a widget will be destroyed and must lose 
    // mouse/keyboard focus
    if (Ec.current_mouse_focus().is<Particle>())
      Ec.reset_mouse_focus();
    auto IndicesMap = S.delete_selection();
    auto new_end = std::remove_if(particles.begin(), particles.end(), 
      [q = 0, &IndicesMap] (auto&) mutable { return IndicesMap[q++] == -1; });
    particles.erase(new_end, particles.end());
    auto NewRelEnd = std::remove_if(connections.begin(), connections.end(), 
      [&IndicesMap] (auto& r) {
        r.m0 = IndicesMap[r.m0];
        r.m1 = IndicesMap[r.m1];
        return r.m0 == -1 || r.m1 == -1;
      });
    connections.erase( NewRelEnd, connections.end() );
  }
  
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

auto make_view(SpringSimApp& state)
{ 
  auto WithLabel = [] (auto&& View, std::string_view str) {
    return hstack {
      text{str}, 
      (decltype(View)&&)(View)
    }.with_align(0.5);
  };
  
  auto MassMod = 
    WithLabel( 
      either {
        state.selection_is_anchor(), 
        slider{ readwrite(&SpringSimApp::get_selection_mass, &SpringSimApp::set_selection_mass) }
        .with_range(1, 1000),
        text {"Inf"}
      }, 
      "Mass" );
  
  auto ForceRead = &SpringSimApp::selected_relations_force;
  auto ForceWrite = &SpringSimApp::set_selected_relations_force;
  
  auto RelForce = 
    slider{ readwrite(ForceRead, ForceWrite) }
    .with_range(1, 400000);
  
  auto RelDamp = 
    slider { readwrite(&SpringSimApp::selected_relation_damp, &SpringSimApp::set_selected_relations_damp) }
    .with_range(0, 1);
  
  auto RelMod = vstack {
    WithLabel(RelForce, "force"),
    WithLabel(RelDamp, "damp")
  };

  auto RightPanel = vstack { 
    combo_box_v{ [] (auto& s) -> auto& { return s.particle_kind; },
               {"Mass", "Anchor", "Pickup"} },
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

inline void run_app() {
  SpringSimApp state;
  auto app = make_app(state, &make_view);
  state.start_audio_render();
  app.run(state);
}