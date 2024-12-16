
#include "../spiral.hpp"
#include <ranges>
#include <shared_mutex>
#include <mutex>

struct Calculator {
  
  
};

auto make_view(Calculator& state)
{
  using namespace spiral::views;
  return grid {
    grid_row{ button{"+", &State::
  };
}

inline void run_app() {
  Calculator state;
  auto app = make_app(state, &make_view);
  app.run(state);
}