# spiral

spiral is a declarative UI library for C++, inspired by the likes of ImGUI, SwiftUI, and xilem. 

It's based on a high level description of UI elements (called "views") that is rebuilt 
on every action, and diffed against the previous version to provide updates, unlike 
"retained" UI library where the updates have to be implemented manually. 

# Hello world

```cpp
#include <spiral/spiral.hpp>
#include <format>

struct State {
  int value = 0;
};

auto make_view(State& state) {
  using namespace spiral::views;
  return vstack{
    text{std::format("counter value : {}", state.value)},
    button{"Increment", [] (auto& s) { ++s.value; }}
  };
}

int main() {
  State state;
  spiral::make_app(state).run();
}
```

# The widgets API 

The underlying logic of the views is implemented by widgets. Unlike other UI framework, 
there is no big abstract `widget` base class. 
spiral avoid cumbersome inheritance hierarchies to favor non-intrusive polymorphism 
and templates. The `widget_ref` and `widget_box` types encapsulate any types deriving 
from the `widget_base` class, which contains only its position and size. 