#include "spiral.hpp"

#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <iostream>

struct SpringSim : app_state, audio_renderer<SpringSim> {
  
  ~SpringSim() {
    stop_audio_render();
  }
  
  auto read_scope() {
    return std::shared_lock{mut};
  }
  
  auto write_scope() {
    return std::unique_lock{mut};
  }
  
  struct Particle {
    bool is_anchor() const { return mass == anchor_mass; }
    static constexpr auto anchor_mass = 10000000000;
    float force = 0.f, vel = 0.f, pos = 0.f, mass = 1.f;
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
    
  void add_particle(bool is_anchor) {
    auto _ = write_scope();
    particles.push_back({0, 0, 0.f, is_anchor ? Particle::anchor_mass : 1.f});
  }
  
  void add_anchor() {
    auto _ = write_scope();
    particles.push_back({0, 0, 0, Particle::anchor_mass});
  }
  
  bool is_anchor(int particle_index) {
    return get_particle(particle_index).mass == Particle::anchor_mass;
  }
  
  void on_connection(int ia, int ib) {
    auto _ = write_scope();
    rels.push_back(Relation{1.f, 0.f, ia, ib});
  }
  
  template <class Range>
  auto remove_particles(Range&& range) 
  {
    auto _ = write_scope();
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
  
  void trigger_particle(int index) {
    auto _ = write_scope();
    get_particle(index).force = 44100 * 400;
  }
  
  void render_audio(audio_output_stream os)
  {
    auto _ = read_scope();
    
    float timestep = 1.f / 44100.f;
    
    float phase = 0;
    
    constexpr float non_lin_coef = 10;
    
    for (auto s : os) 
    {
      for (auto r : rels) {
        auto& p0 = particles[r.ia];
        auto& p1 = particles[r.ib];
        auto lin_delta = p0.pos - p1.pos;
        //auto delta = lin_delta;
        auto delta = lin_delta + std::pow(lin_delta, 3) / 3;// * std::sqrt( std::abs((non_lin_coef + lin_delta) / non_lin_coef) );
        auto vdelta = p0.vel - p1.vel;
        vdelta = vdelta * std::sqrt( (non_lin_coef + std::abs(vdelta)) / non_lin_coef );
        p0.force -= delta * r.force + vdelta * r.damp;
        p1.force += delta * r.force + vdelta * r.damp;
      }
      
      for (auto& p : particles) {
        if (p.is_anchor())
          continue;
        p.vel += (p.force * timestep) / p.mass;
        p.pos = p.pos + p.vel * timestep;
        // p.pos = std::clamp( p.pos + p.vel * timestep, -1.f, 1.f );
        std::cout << p.pos << std::endl;
      }
      
      float sum = 0;
      for (auto& p : particles) {
        if (!p.is_anchor()) 
          sum += p.pos;
        p.force = 0;
      }

      //sum /= particles.size();
      
      // sum = std::sin(phase);
      // phase += 2 * 3.14 * 440 * 1.f / 44100.f;
      sum *= output_amp;
      s[0] = sum;
      s[1] = sum;
    }
  }
  
  std::shared_mutex mut;
  std::vector<Particle> particles;
  std::vector<Relation> rels;
  std::vector<int> pick_ups;
  float output_amp = 1;
};