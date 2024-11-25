#pragma once

#include "spiral.hpp"

struct TextureSynthesis {
  
  void load_image() {
    auto path = open_file_dialog();
    if (!path)
      return;
    auto res = load_image_from_file(*path);
    if (res)
      display_image = std::move(*res);
  }
  
  image_data display_image;
};

auto make_texture_synth(TextureSynthesis& state)
{
  auto img_size = min(state.display_image.size(), {600, 400});
  
  return vstack {
    text{ "Texture Synthesis from examplar"},
    trigger_button { "Load texture", [] (auto& s) { s.load_image(); } },
    image { state.display_image, img_size }
  };
}

inline void run_texture_synth() {
  TextureSynthesis state;
  auto app = make_app(state, &make_texture_synth);
  app.run(state);
}