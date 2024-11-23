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