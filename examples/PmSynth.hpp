
#include "../spiral.hpp"
#include <ranges>
#include <shared_mutex>
#include <mutex>

struct PmSynth : audio_renderer<PmSynth>, app_state
{
  static constexpr int num_osc = 4;
  
  auto read_scope() { return std::shared_lock{mut}; }
  auto write_scope() { return std::unique_lock{mut}; }
  
  void render_audio(audio_output_stream os)
  {
    auto _ = read_scope();
    
    for (auto s : os) 
    {
      float res = 0;
      int OscIndex = 0;
      for (auto& o : osc)
      {
        float acc = 0;
        for (int k = 0; k < num_osc; ++k)
          acc += osc[k] * mod_matrix[OscIndex][k];
        o = std::sin( osc_phase[OscIndex] * 2 * 3.14 * 440 * freq_ratio[OscIndex] + acc );
        osc_phase[OscIndex] += 1.f / 44100.f;
        res += o * osc_vol[OscIndex];
        ++OscIndex;
      }
      s[0] = res;
      s[1] = res;
    }
  }
  
  std::shared_mutex mut;
  float mod_matrix[num_osc][num_osc] = {0};
  float osc_phase[num_osc] {0};
  float osc[num_osc] = {0.0};
  float freq_ratio[num_osc] = {1.0};
  float osc_vol[num_osc] = {1.0, 0.0};
  
  bool show_modulations;
};

auto make_pm_synth(PmSynth& state)
{
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