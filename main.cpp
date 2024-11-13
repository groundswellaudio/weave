#include "spiral.hpp"

struct AppState {
  float x = 0.3, y = 0.5, z = 0.9;
  bool flag = false;
  std::vector<float> vals = {0.2, 0.3, 0.4};
};

auto make_demo_app(AppState& state)
{
  return vstack{ 
    /* 
    slider{%lens(^(state.x))}, 
    hstack {
      slider{%lens(^(state.y))}, 
      slider{%lens(^(state.z))}, 
      text{"hello"}
    },  */
    toggle_button{ %lens(^(state.flag)), "Flag"}, 
    for_each{
      %lens(^(state.vals)),
      [] (auto lens, auto& state) {
        return slider{lens};
      }
    },
    maybe {
      state.flag, 
      slider{%lens(^(state.x))}
    }, 
    trigger_button{ "add slider!", 
      [] (AppState& state) {
        state.vals.push_back(0.8);
      }}
      
    //toggle_button{}
  };
}

int main()
{
  AppState state;
  auto app = make_app(state, &make_demo_app);
  app.run(state);
}