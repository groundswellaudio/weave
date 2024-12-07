#include "spiral.hpp"
#include "examples/PmSynth/PmSynth.hpp"

/* 
struct AppState : audio_renderer<AppState> {
  
  void render_audio(audio_output_stream os) 
  {
  }
  
  float x = 0.3, y = 0.5, z = 0.9;
  bool flag = false;
  std::vector<float> vals = {0.2, 0.3, 0.4};
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
}; */

int main()
{
  run_app();
}