
#include "../spiral.hpp"
#include <ranges>
#include <shared_mutex>
#include <mutex>

struct Calculator {
  
  
};

auto make_pm_synth(PmSynth& state)
{
  
  vstack {
    text{ state.display }.with_size({100, 50})
    hstack {
      calc_button{"1", [n] (auto& s) { s.on_num_press(n); }}, calc_button{"2"}, calc_button{"3"}
    }
  }
  
  auto slider_row = [&] (int row) {
    return hstack {
      for_each{
        iota(state.num_osc),
        [row] (int col) {
          return slider{ MLens(x0.mod_matrix[row][col]) };
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
    return hstack {
      numeric_field { MLens(x0.freq_ratio[row]), numeric_field_properties{0, 100} },
      slider { MLens(x0.osc_vol[row]) }
    };
  };
  
  auto left_panel = vstack {
    for_each { 
      iota(state.num_osc),
      osc_panel_row
    }
  }.with_interspace(10);
  
  return hstack {
    left_panel, mod_matrix
  }.with_interspace(30).with_margin({30, 30});
}

inline void run_pm_synth() {
  PmSynth state;
  state.start_audio_render();
  auto app = make_app(state, &make_pm_synth);
  app.run(state);
}