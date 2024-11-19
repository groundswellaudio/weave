#include "spiral.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "deps/miniaudio/miniaudio.h"

struct AppState : audio_renderer<AppState> {
  
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
  
  struct internal {
    float delta;
  };
  
  internal intern;
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
  
  return s; /* vstack{ s 
    slider{%lens(^(state.x))}, 
    vstack {
      slider{%lens(^(state.y))},
      slider{%lens(^(state.z))}, 
      //text{"hello"}
    }, */ 
    /* 
    toggle_button{%lens(^(state.flag)), "Flag"},
    for_each{
      %lens(^(state.vals)),
      [] (auto lens, auto& state) {
        return slider{lens}.with_range(30, 20000);
      }
    },
    maybe {
      state.flag, 
      slider{%lens(^(state.x))}
    }, */ 
    /* 
    trigger_button{ "add slider!", 
      [] (AppState& state) {
        state.vals.push_back(0.8);
      }} */ 
      
    //toggle_button{}
  /* }
  .with_interspace(8)
  .with_margin({10, 10});*/ 
}

int main()
{
  AppState state;
  state.start_audio_render();
  auto app = make_app(state, &make_demo_app);
  app.run(state);
}