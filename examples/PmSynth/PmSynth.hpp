
#include "spiral.hpp"
#include <shared_mutex>
#include <mutex>
#include <span>
#include <numbers>

struct PmSynth;

template <class T>
struct modulable {
  
  auto modulated() const { return value * modulation; }
  
  auto& operator=(T val) { value = val; return *this; }
  operator T() const { return value; }
  
  T value;
  T modulation = 1;
};

std::span<const tuple<const char*, modulable<float>*(*)(PmSynth&)>> mod_slots();

struct PmSynth : audio_renderer<PmSynth>, app_state
{
  static constexpr int num_osc = 4;
  
  decltype(auto) apply_read(auto&& fn) {
    auto _ = std::shared_lock{mut};
    return (fn(*this));
  }
  
  void apply_write(auto&& fn) {
    auto _ = std::unique_lock{mut};
    fn(*this);
  }
  
  void render_audio(audio_output_stream os)
  {
    auto _ = std::shared_lock{mut};
    float dt = 1.f / 44100.f;
    for (auto s : os) 
    {
      for (auto& r : mod_matrix)
        for (auto& e : r)
          e.modulation = 1;
      for (auto& r : freq_ratio)
        r.modulation = 1;
      for (auto& o : osc_vol)
        o.modulation = 1;
      
      for (auto& l : lfos)
      {
        if (l.dest.dest)
          l.dest.dest->modulation += l.mod * std::sin(l.phi * 2 * std::numbers::pi);
        l.phi += dt * l.freq;
        if (l.phi >= 1)
          l.phi -= 2;
      }
      
      float res = 0;
      int OscIndex = 0;
      for (auto& o : osc)
      {
        float acc = 0;
        for (int k = 0; k < num_osc; ++k)
          acc += osc[k] * mod_matrix[OscIndex][k].modulated();
        o = std::sin( osc_phase[OscIndex] * 2 * std::numbers::pi + acc );
        res += o * osc_vol[OscIndex].modulated();
        osc_phase[OscIndex] += dt * freq_ratio[OscIndex].modulated() * 440;
        if (osc_phase[OscIndex] >= 1)
          osc_phase[OscIndex] -= 2;
        ++OscIndex;
      }
      s[0] = res;
      s[1] = res;
    }
  }
  
  void set_lfo_dest(int index, int dest) {
    auto [name, fn] = mod_slots()[dest];
    lfos[index].dest = ModDestination{ fn ? fn(*this) : nullptr, dest, name };
  }
  
  std::shared_mutex mut;
  modulable<float> mod_matrix[num_osc][num_osc] = {0};
  modulable<float> freq_ratio[num_osc] = {1.0};
  modulable<float> osc_vol[num_osc] = {1.0, 0.0};
  float osc_phase[num_osc] {0};
  float osc[num_osc] = {0.0};
  
  struct ModDestination {
    modulable<float>* dest = nullptr;
    int index = 0; 
    std::string mod_name; 
  };
    
  struct lfo_params {
    enum Kind {
      sqr, sin, tri
    };
    
    unsigned kind = 1;   
    float freq = 10, mod = 1, phi = 0;
    
    ModDestination dest;
  };
  
  lfo_params lfos[2];
  float lfos_phases[2] {0};
  
  bool show_modulations;
};

std::span<const tuple<const char*, modulable<float>*(*)(PmSynth&)>> mod_slots()
{ 
  using Self = PmSynth;
  
  constexpr auto num_osc = PmSynth::num_osc;
  
  constexpr auto fm_slots = [] () {
    expr_list el;
    for (int index : iota(num_osc))
      for (int k : iota(num_osc)) {
        std::meta::ostream os;
        os << "Fmod " << k + 1 << "->" << index + 1;
        auto lit = make_literal_expr(os);
        auto elem = ^[k, lit, index] ( tuple{ %lit, +[] (Self& s) { return &s.mod_matrix[index][k]; }} );
        push_back(el, elem);
      }
    return el;
  };
  
  constexpr auto freq_slots = [] () {
    expr_list el;
    for (auto index : iota(num_osc)) {
      std::meta::ostream os;
      os << "Freq " << index + 1;
      auto lit = make_literal_expr(os);
      auto e = ^[index, lit] ( tuple{%lit, +[] (Self& s) { return &s.freq_ratio[index]; }} );
      push_back(el, e);
    }
    return el;
  };
  
  constexpr auto vol_slots = [] () {
    expr_list el;
    for (auto index : iota(num_osc)) {
      std::meta::ostream os;
      os << "Vol " << index + 1;
      auto lit = make_literal_expr(os);
      auto e = ^[lit, index] ( tuple{%lit, +[] (Self& s) { return &s.osc_vol[index]; }} );
      push_back(el, e);
    }
    return el;
  };
   
  static constexpr tuple<const char*, modulable<float>*(*)(Self&)> slots[] {
    tuple<const char*, modulable<float>*(*)(Self&)>{"none", nullptr}, 
    %...vol_slots()...,
    %...freq_slots()...,
    %...fm_slots()...
  };
  
  //return &slots;
  return {&slots[0], std::size(slots)};
}

auto make_view(PmSynth& state)
{
  using namespace views;
  using State = PmSynth;
  
  auto slider_row = [&] (int row) {
    return hstack {
      for_each{
        iota(state.num_osc),
        [row] (int col) {
          auto lens = [row, col] (State& s) -> auto& { return s.mod_matrix[row][col]; };
          return slider{ lens };
        }
      }
    }.with_interspace(10);
  };
  
  auto mod_matrix = vstack{ 
    for_each{ 
      iota(state.num_osc),
      slider_row
    }
  }.with_interspace(10);
  
  auto osc_panel_row = [] (int row) {
    auto lens_freq = [row] (State& s) -> auto& { return s.freq_ratio[row]; };
    auto lens_vol = [row] (State& s) -> auto& { return s.osc_vol[row]; };
    
    return hstack {
      numeric_dial { lens_freq }.range(0, 100),
      slider { lens_vol }
    };
  };
  
  auto lfo = [] (int index) {
    auto baselens = [index] (auto& s) -> auto& { return s.lfos[index]; };
    auto kind = [baselens] (auto& s) -> auto& { return baselens(s).kind; };
    auto freq = [baselens] (auto& s) -> auto& { return baselens(s).freq; };
    //auto mod_dest = [index] (auto &s) { s.set_lfo_dest(index, val); };
    
    return vstack {
      combo_box{ kind, {"Square", "Sine", "Triangle"} },
      slider{ freq }.range(0.2, 20),
      combo_box{ readwrite( [index] (auto& s) { return s.lfos[index].dest.index; },
                            [index] (auto& s, int val) { s.set_lfo_dest(index, val); } ),
                 mod_slots() | std::views::transform([] (auto& e) { return get<0>(e); }) }
    };
  };
  
  auto lfo_panel = hstack {
    lfo(0), lfo(1)
  };
  
  auto left_panel = vstack {
    for_each { 
      iota(state.num_osc),
      osc_panel_row
    }, 
    lfo_panel
  }.with_interspace(10);
  
  return hstack {
    left_panel, mod_matrix
  }.with_interspace(30).with_margin({30, 30});
}

inline void run_app() {
  PmSynth state;
  state.start_audio_render();
  auto app = make_app(state, &make_view);
  app.run(state);
}