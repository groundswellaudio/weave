
#include <spiral.hpp>
#include "PmSynth.hpp"

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
    }.interspace(10);
  };
  
  auto mod_matrix = vstack{ 
    for_each{ 
      iota(state.num_osc),
      slider_row
    }
  }.interspace(10);
  
  auto osc_panel_row = [] (int row) {
    auto lens_freq = [row] (State& s) -> auto& { return s.freq_ratio[row]; };
    auto lens_vol = [row] (State& s) -> auto& { return s.osc_vol[row]; };
    auto lens_fold = [row] (State& s) -> auto& { return s.fold_thres[row]; };
    
    return hstack {
      numeric_dial { lens_freq }.range(0, 100),
      slider{ lens_vol }, 
      slider{ lens_fold }
    };
  };
  
  auto lfo = [] (int index) {
    auto baselens = [index] (auto& s) -> auto& { return s.lfos[index]; };
    auto kind = [baselens] (auto& s) -> auto& { return baselens(s).kind; };
    auto freq = [baselens] (auto& s) -> auto& { return baselens(s).freq; };
    auto mod = [baselens] (auto& s) -> auto& { return baselens(s).mod; };
    
    //auto mod_dest = [index] (auto &s) { s.set_lfo_dest(index, val); };
    
    return vstack {
      combo_box{ kind, {"Square", "Sine", "Triangle"} },
      slider{ freq }.range(0.2, 20),
      combo_box{ readwrite( [index] (auto& s) { return s.lfos[index].dest.index; },
                            [index] (auto& s, int val) { s.set_lfo_dest(index, val); } ),
                 mod_slots() | std::views::transform([] (auto& e) { return get<0>(e); }) },
      slider{ mod }.range(0, 2)
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
  }.interspace(10);
  
  auto top_panel = combo_box {
    readwrite( [] (auto& s) { return s.current_device_index(); },
               &State::set_audio_device ),
    audio_output_devices()
  };
  
  return vstack {
    std::move(top_panel),
    hstack {
      left_panel, mod_matrix
    }.interspace(30).margin({30, 30})
  }.background(rgb_f(colors::gray) * 0.35);
}

inline void run_app() {
  PmSynth state;
  state.start_audio_render();
  auto app = make_app(state, &make_view);
  app.run(state);
}