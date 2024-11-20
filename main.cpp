#include "spiral.hpp"
#include <ranges>

#define MINIAUDIO_IMPLEMENTATION
#include "deps/miniaudio/miniaudio.h"

struct AppState : audio_renderer<AppState> {
  
  void render_audio(audio_output_stream os) 
  {
  }
  
  float x = 0.3, y = 0.5, z = 0.9;
  bool flag = false;
  std::vector<float> vals = {0.2, 0.3, 0.4};
};

struct PmSynth : audio_renderer<PmSynth> {
  
  void render_audio(audio_output_stream os) 
  {
    /* 
    for (auto s : os) {
      auto x = std::sin(intern.delta * 2 * 3.14 * 440);
      intern.delta += 1.f / 44100.f;
      s[0] = x;
      s[1] = x; 
    } */ 
  }
  
  static constexpr int num_osc = 4;
  float mod_matrix[num_osc][num_osc] = {0};
  float osc[4] = {0.0};
  float osc_vol[4] = {1.0};
};

auto make_demo_app(AppState& state)
{
  auto s = scrollable {
    vec2f{ 60, 50 },
    vstack {
      slider{ %lens(^(state.x)) },
      slider{ %lens(^(state.y)) }, 
      slider{ %lens(^(state.z)) }
    }.with_margin({20, 20})
     .with_interspace(30)
  };
  return s;  
};

#define MLens(Postfix) [=] (auto& s) -> decltype(auto) { return (s Postfix); }

auto make_pm_synth(PmSynth& state)
{
  auto slider_row = [&] (int row) {
    return hstack {
      simple_for_each{
        std::ranges::iota_view(0, state.num_osc),
        [row] (int col) {
          return slider{ MLens(.mod_matrix[row][col]) };
        }
      }
    }.with_interspace(10);
  };
  
  auto mod_matrix = vstack{ 
    simple_for_each{ 
      std::ranges::iota_view(0, state.num_osc),
      slider_row
    }
  }.with_interspace(10);
  
  auto vol = vstack {
    simple_for_each { 
      std::ranges::iota_view(0, state.num_osc),
      [] (int index) { 
        return slider { MLens(.osc_vol[index]) };
      }
    }
  }.with_interspace(10);
  
  return hstack {
    vol, mod_matrix
  }.with_interspace(30);
}

int main()
{
  PmSynth state;
  state.start_audio_render();
  auto app = make_app(state, &make_pm_synth);
  app.run(state);
}