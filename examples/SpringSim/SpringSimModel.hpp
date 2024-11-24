#include "spiral.hpp"

#include <algorithm>

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
  
  template <class Range>
  auto remove_particles(Range&& range) 
  {
    std::vector<int> new_indices;
    new_indices.resize(particles.size(), 0);
    for (auto r : range)
      new_indices[r] = -1;
    int k = 0;
    for (auto& i : new_indices) {
      if (i == 0)
        i = k++;
    }
    auto new_end = std::remove_if(particles.begin(), particles.end(), [q = 0, &new_indices] (auto&) mutable {
      return new_indices[q++] == -1;
    }); 
    particles.erase(new_end, particles.end());
    
    // Update edges
    rels.erase( std::remove_if(rels.begin(), rels.end(), [&] (auto& r) {
      r.ia = new_indices[r.ia];
      r.ib = new_indices[r.ib];
      return r.ia == -1 || r.ib == -1;
    }), rels.end());
    
    return new_indices;
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