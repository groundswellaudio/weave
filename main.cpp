#include "spiral.hpp"

struct AppState {
  float x = 0.3, y = 0.5, z = 0.9;
  bool flag = false;
};

auto make_demo_app(AppState& state)
{
  return vstack{ 
    slider{%lens(^(state.x))}, 
    hstack {
      slider{%lens(^(state.y))}, 
      slider{%lens(^(state.z))}, 
      text{"hello"}
    }, 
    toggle_button{%lens(^(state.flag)), "Flag"}
  };
}

int main()
{
  AppState state;
  auto app = make_app(state, &make_demo_app);
  app.run(state);
}