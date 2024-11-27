#pragma once

#include "spiral.hpp"

#include <ranges>
#include <random>

auto iota(int a, int b) {
  return std::ranges::iota_view(a, b);
}

template <class T>
struct padded_image : image<T> {
  
  constexpr auto shape() const {
    return image<T>::shape() - margin * 2;
  }
  
  constexpr auto& operator()(this auto& self, vec2i idx) {
    return ((image<T>&)self)(idx + self.margin);
  }
  
  constexpr auto& operator()(this auto& self, int y, int x) {
    return self({y, x});
  }
  
  constexpr auto data() const {
    return &(*this)({0, 0});
  }
  
  void set_padding(vec2i pad) { margin = pad; }
  
  vec2i margin;
};

template <class I>
auto patch_distance(const I& a, vec2<int> idxa, const I& b, vec2<int> idxb, int patch_hs) 
{
  decltype(distance_squared(a(0, 0), b(0, 0))) sum = 0;
  for (auto y : iota(-patch_hs, patch_hs + 1))
  {
    for (auto x : iota(-patch_hs, patch_hs + 1))
    {
      sum += distance_squared( a(idxa + vec2i{y, x}), b(idxb + vec2i{y, x}) );
    }
  }
  return sum;
}

using search_map = image<tuple<vec2i, float>>;

template <class I>
void patch_match_search(const I& examplar, I& generated, search_map& Map, int patch_hs)
{
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution<int> rand{-10000000, 1000000};
    
  for (auto y : iota(generated.shape().y))
  {
    for (auto x : iota(generated.shape().x))
    {
      float min_dist = Map(y, x).m1;
      vec2i origin = Map(y, x).m0;
      
      for(int distance = 64; distance > 1; distance /= 2)
      {
        auto nx = origin.x + rand(gen) % distance;
        auto ny = origin.y + rand(gen) % distance;
        
        nx = std::clamp(x, 0, examplar.shape().x - 1);
        ny = std::clamp(y, 0, examplar.shape().y - 1);
        
        auto new_index = vec2i{nx, ny};
        
        auto patch_dist = patch_distance(examplar, new_index, generated, {y, x}, patch_hs);
        if (patch_dist < min_dist) {
          min_dist = patch_dist;
          origin = new_index;
        }
      }
      
      Map(y, x) = tuple{origin, min_dist};
    }
  }
}

template <class I>
void patch_match_propagation(I& examplar, I& generated, search_map& Map, int patch_hs) 
{
  for (auto y : iota(1, Map.shape().y))
  {
    for (auto x : iota(1, Map.shape().x))
    {
      auto try_propagate = [&] (int dy, int dx) {
        auto patch_dist = patch_distance(examplar, Map(y - dy, x - dx).m0, generated, vec2i{y, x}, patch_hs);
        if (patch_dist < Map(y, x).m1) {
          Map(y, x).m1 = patch_dist;
          Map(y, x).m0 = Map(y-dy, x-dx).m0;
        }
      };
      
      try_propagate(0, 1);
      try_propagate(1, 0);
    }
  }
}

template <class I>
void patch_match_synthesize(I& source, I& generated, search_map& map)
{
  for (auto y : iota(map.shape().y))
  {
    for (auto x : iota(map.shape().x))
    {
      generated(y, x) = source(map(y, x).m0);
    }
  }
}

template <class T>
void patch_match_update_distance(const image<T>& source, image<T>& generated, search_map& map, int patch_hs)
{
  for (auto y : iota(map.shape().y))
    for (auto x : iota(map.shape().x))
      map(y, x).m1 = patch_distance(source, map(y, x).m0, generated, {y, x}, patch_hs);
}


template <class I>
void fill_with_noise(I& img) {
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution<typename I::pixel_type::scalar> 
    distrib(0, I::pixel_type::norm());
  for (auto& p : img) {
    for (auto k : iota(I::pixel_type::channels))
      p[k] = distrib(gen);
  }
}

struct TextureSynthesis {
  
  TextureSynthesis() 
  : generated{vec2i{600, 400}}
  {
    map.reshape({600, 400});
    fill_with_noise(generated);
    generated.set_padding({10, 10});
  }
  
  void load_image() {
    auto path = open_file_dialog();
    if (!path)
      return;
    auto res = load_image_from_file(*path);
    if (res)
      display = std::move(*res);
    examplar = padded_image{display, {10, 10}};
    reset_search_map();
  }
  
  void reset_search_map() {
    std::random_device rd;
    std::default_random_engine gen(rd());
    std::uniform_int_distribution<int> gen_y (0, examplar.shape()[0]);
    std::uniform_int_distribution<int> gen_x (0, examplar.shape()[1]);
    for (auto& entry : map) {
      entry.m0 = vec2i{gen_y(gen), gen_x(gen)};
      entry.m1 = 1000000000000.f;
    }
  }
  
  void run_synth_step() {
    patch_match_search(examplar, generated, map, 5);
    patch_match_propagation(examplar, generated, map, 5);
    patch_match_synthesize(examplar, generated, map);
    patch_match_update_distance(examplar, generated, map, 5);
  }
  
  using u8 = unsigned char;
  
  padded_image<rgba<u8>> generated;
  padded_image<rgba<u8>> examplar;
  search_map map;
  
  image<rgba<u8>> display;
};

auto make_texture_synth(TextureSynthesis& state)
{
  auto img_size = min(state.examplar.shape(), {600, 400});
  auto ImgPadding = 10;
  
  return vstack {
    text{ "Texture Synthesis from examplar"},
    trigger_button { "Load texture", [] (auto& s) { s.load_image(); } },
    trigger_button{ "Synthesize", [] (auto& s) { s.run_synth_step(); } },
    hstack {
      // views::image { state.display },
      views::image { state.generated }.with_corner_offset( {ImgPadding, ImgPadding} )
    }
  };
}

inline void run_texture_synth() {
  TextureSynthesis state;
  auto app = make_app(state, &make_texture_synth);
  app.run(state);
}