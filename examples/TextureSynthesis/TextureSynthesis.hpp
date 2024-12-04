#pragma once

#include "spiral.hpp"

#include <ranges>
#include <random>
#include <future>

auto iota(int a, int b) {
  return std::ranges::iota_view(a, b);
}

template <class T>
struct padded_image {
  
  template <class P>
  using pixel_template = image<T>::template pixel_template<P>;
  
  using pixel_type = T;
  
  bool empty() const { return img.empty(); }
  
  constexpr auto shape() const {
    return img.shape() - margin * 2;
  }
  
  void reshape(vec2i new_shape) {
    img.reshape(new_shape + margin * 2);
  }
  
  constexpr auto& operator()(this auto& self, vec2i idx) {
    return self.img(idx + self.margin);
  }
  
  template <class Pixel>
  image<Pixel> to() const {
    image<Pixel> res;
    res.reshape(shape());
    for (int x0 : iota(shape()[0]))
      for (int x1 : iota(shape()[1]))
        res(x0, x1) = static_cast<Pixel>((*this)(x0, x1));
    return res;
  }
  
  template <class I>
  constexpr bool operator==(const I& o) const {
    if (shape() != o.shape())
      return false;
    for (int x0 = 0; x0 < shape()[0]; ++x0)
      for (int x1 = 0; x1 < shape()[1]; ++x1)
        if ((*this)(vec2i{x0, x1}) != o(vec2i{x0, x1}))
          return false;
    return true; 
  }
  
  constexpr auto& operator()(this auto& self, int y, int x) {
    return self({y, x});
  }
  
  void set_padding(vec2i pad) { margin = pad; }
  
  image<T> img; 
  vec2i margin;
};

template <class T>
auto wrap(T x, T x_min, T x_max){
  if (x < x_min)
    return x_max - (x_min - x) % (x_max - x_min);
  else if (x >= x_max)
    return x_min + (x - x_min) % (x_max - x_min);
  else
    return x;
}

template <class I>
auto patch_distance(const I& a, vec2<int> idxa, const I& b, vec2<int> idxb, int patch_hs) 
{
  assert( idxa.x < a.shape().x );
  assert( idxa.y < a.shape().y );
  assert( idxb.x < b.shape().x );
  assert( idxb.y < b.shape().y );
  
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

template <class I>
auto mean(const I& i) {
  using RT = typename I::template pixel_template<float>;
  RT res {0};
  for (auto y : iota(i.shape()[0]))
  {
    RT acc{0};
    for (auto x : iota(i.shape()[1]))
      acc += i(y, x);
    acc /= (float) i.shape()[1];
    res += acc;
  }
  res /= (float) i.shape()[0];
  return res;
}

template <class I, class M>
auto variance(const I& i, M mean) {
  M res {0};
  for (auto y : iota(i.shape()[0]))
  {
    M acc{0};
    for (auto x : iota(i.shape()[1])) {
      M delta = i(y, x) - mean;
      delta *= delta;
      acc += delta;
    }
    acc /= i.shape()[1];
    res += acc;
  }
  res /= i.shape()[0];
  return res;
}

template <class I, class Fn>
void for_each_pixel(I& image, Fn fn) {
  for (auto y : iota(image.shape()[0])) 
    for (auto x : iota(image.shape()[1]))
      fn( image(y, x), vec2i{y, x} );
}

template <class I>
void match_distribution(const I& examplar, I& generated) {
  auto mg = mean(generated);
  auto dg = sqrt( variance(generated, mg) );
  auto me = mean(examplar);
  auto de = sqrt( variance(examplar, me) );
  for_each_pixel( generated, [&] (auto& pix, ignore) {
    pix = ((pix - mg) * de) / dg + me;
    for (int k = 0; k < I::pixel_type::channels; ++k)
      pix[k] = std::clamp(pix[k], typename I::pixel_type::scalar{0}, I::pixel_type::norm());
  });
}

using search_map = image<tuple<vec2i, float>>;

template <class I>
void patch_match_search(const I& examplar, I& generated, search_map& Map, int patch_hs)
{ 
  assert( Map.shape() == generated.shape() );
  
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution<int> rand{-1000, 1000};
  
  for_each_pixel( generated, [&] (auto& pix, vec2i idx) {
    auto y = idx[0];
    auto x = idx[1];
    
    float min_dist = Map(idx).m1;
    vec2i origin = Map(idx).m0;
    
    for(int distance = 64; distance > 1; distance /= 2)
    {
      auto rx = rand(gen) % distance;
      auto ry = rand(gen) % distance;
      auto nx = origin.x + rx;
      auto ny = origin.y + ry;
      nx = wrap(nx, 0, examplar.shape().x - 1);
      ny = wrap(ny, 0, examplar.shape().y - 1);
      
      auto new_index = vec2i{nx, ny};
      
      auto patch_dist = patch_distance(examplar, new_index, generated, idx, patch_hs);
      if (patch_dist < min_dist) {
        min_dist = patch_dist;
        origin = new_index;
      }
    }
    
    Map(idx) = tuple{origin, min_dist};
  });
}

template <bool B, class R>
constexpr auto maybe_reverse(R range) {
  if constexpr (B)
    return std::ranges::reverse_view(range);
  else
    return range;
}

template <class Range>
struct drop_first_n {
    
  constexpr auto begin(this auto& self) { 
    auto it = self.range.begin(); 
    int k = self.skip; 
    while(k--) {
      ++it;
    }
    return it;
  }
  
  constexpr auto end(this auto& self) { return self.range.end(); }

  Range range;
  int skip = 1;
};

template <class Range>
constexpr auto drop_first(Range&& range, int skip) {
  return drop_first_n<Range>{(Range&&)range, skip};
}

template <int Sign, class I>
void patch_match_propagation(I& examplar, I& generated, search_map& Map, int patch_hs) 
{
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_int_distribution distrib{0, 100};
  
  static_assert( Sign == -1 || Sign == +1 );
  auto yrange = maybe_reverse<Sign == -1>(iota(0, Map.shape()[0]));
  auto xrange = maybe_reverse<Sign == -1>(iota(0, Map.shape()[1]));
  
  for (auto y : drop_first(yrange, 1))
  {
    for (auto x : drop_first(xrange, 1))
    {
      // skip propagation for 30 percent of pixels
      if (distrib(gen) > 70)
        continue;
      
      auto try_propagate = [&] (int dy, int dx) {
        
        auto new_idx = Map(y - dy, x - dx).m0 + vec2i{dy, dx};
        // Don't propagate if we reached the end/beginning of examplar
        if constexpr (Sign > 0) {
          if (new_idx[0] == examplar.shape()[0]
             || new_idx[1] == examplar.shape()[1])
            return;
        }
        else {
          if (new_idx[0] == -1 || new_idx[1] == -1)
            return;
        }
        
        auto patch_dist = patch_distance(examplar, new_idx, generated, vec2i{y, x}, patch_hs);
        if (patch_dist < Map(y, x).m1) {
          Map(y, x).m1 = patch_dist;
          Map(y, x).m0 = new_idx;
        }
      };
      
      try_propagate(0, Sign);
      try_propagate(Sign, 0);
    }
  }
}

template <class R1, class R2>
void for_point_in(R1&& a, R2&& b, auto Fn) {
  using scalar = decltype(*a.begin());
  for (auto x0 : a)
    for (auto x1 : b)
      Fn(vec2<scalar>{x0, x1});
}


template <class I>
void patch_match_synthesize(I& source, I& generated, search_map& map)
{
  for_each_pixel( generated, [&] (auto& pix, vec2i idx) {
    pix = source(map(idx).m0);
  });
  
  /* 
  for_point_in( iota(1, generated.shape()[0] - 1), iota(1, generated.shape()[1] - 1), 
                [&] (vec2i idx) {
                  auto acc = [&] (vec2i delta) {
                    generated(idx) += source(map(idx + delta).m0 - delta);
                  };
                  generated(idx) += generated(idx);
                  acc({1, 1});
                  acc({1, 0});
                  acc({1, -1});
                  acc({0, 1});
                  acc({0, -1});
                  acc({-1, 1});
                  acc({-1, 0});
                  acc({-1, -1});
                  generated(idx) /= 10;
                }); */ 
}

template <class I>
void patch_match_update_distance(const I& source, const I& generated, search_map& map, int patch_hs)
{
  for_each_pixel( map, [&] (auto& elem, vec2i idx) {
    elem.m1 = patch_distance(source, elem.m0, generated, idx, patch_hs);
  });
}

consteval type distrib_type(type scalar) {
  if (is_floating_point(scalar))
    return instantiate( ^std::uniform_real_distribution, {scalar} );
  else
    return instantiate( ^std::uniform_int_distribution, {scalar} );
}

template <class I>
void fill_with_noise(I& img) {
  std::random_device rd;
  std::default_random_engine gen(rd());
  using scalar = typename I::pixel_type::scalar;
  using distrib_t = %distrib_type(^scalar);
  distrib_t
    distrib(0, I::pixel_type::norm());
  for_each_pixel( img, [&] (auto& pix, ignore) {
    for (auto k : iota(I::pixel_type::channels))
      pix[k] = distrib(gen);
  });
}

void dump_search_map(const search_map& m) {
  for (auto& e : m)
    std::cout << e.m0 << " ";
}

struct TextureSynthesis {
  
  static constexpr auto max_gen_sz = vec2i{1080, 1920};
  
  TextureSynthesis() 
  : generated{vec2i{600, 800}}
  {
    generated.set_padding({20, 20});
    map.reshape(generated.shape());
    fill_with_noise(generated);
    reset_search_map();
  }
  
  bool read_scope() { return true; }
  bool write_scope() { return true; }
  
  void load_image() {
    auto path = open_file_dialog();
    if (!path)
      return;
    auto res = load_image_from_file(*path);
    if (!res)
      return;
    //display = res->to<rgba<unsigned char>>();
    examplar = padded_image{res->to<rgb<float>>(), {15, 15}};
    reset_search_map();
    refresh_examplar = true;
  }
  
  void save_image() {
    auto path = save_file_dialog();
    if (!path)
      return;
    auto saved = generated.to<rgba<unsigned char>>();
    auto success = save_image_to_file(*path, saved);
    assert( success );
  }
  
  void set_output_shape(vec2i new_shape) {
    generated.reshape(new_shape);
    map.reshape(new_shape);
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
    auto fn = [this] {
      int patch_hs = 8;
      patch_match_search(examplar, generated, map, patch_hs);
      progress = 0.25;
      if (!flip_propagation)
        patch_match_propagation<1>(examplar, generated, map, patch_hs);
      else
        patch_match_propagation<-1>(examplar, generated, map, patch_hs);
      flip_propagation = !flip_propagation; 
      progress = 0.5;
      patch_match_synthesize(examplar, generated, map);
      progress = 0.6;
      match_distribution(examplar, generated);
      progress = 0.75;
      patch_match_update_distance(examplar, generated, map, patch_hs);
      progress = -1;
      refresh_display = true;
    };
    progress = 0;
    synth_task = std::async(fn);
  }
  
  bool is_working() const {
    return progress >= 0.f;
  }
  
  using u8 = unsigned char;
  
  padded_image<rgb<float>> generated;
  padded_image<rgb<float>> examplar;
  search_map map;
  bool flip_propagation = true;
  
  std::atomic<float> progress = -1;
  std::future<void> synth_task;
  std::atomic<bool> refresh_display = false;
  bool refresh_examplar = false;
};

auto make_view(TextureSynthesis& state)
{
  using State = TextureSynthesis;
  
  auto img_size = min(state.examplar.shape(), {1600, 800});
  auto ImgPadding = 10;
  
  using namespace views;
  
  bool refresh_examplar = std::exchange(state.refresh_examplar, false);
  bool refresh_display = state.refresh_display.exchange(false);
  
  constexpr auto size_dial = [] (int k) {
    return numeric_field {
      readwrite( [k] (auto& s) { return s.generated.shape()[k]; }, 
                 [k] (auto& s, int val) { auto new_shape = s.generated.shape(); 
                                          new_shape[k] = val;
                                          s.set_output_shape(new_shape);
                                        }
               )
    }.range(100, 5000);
  };
  
  return vstack {
    text{ "Texture Synthesis from examplar"},
    trigger_button { "Load texture", &State::load_image }
    .disable_if(state.is_working()),
    trigger_button{ "Synthesize", &State::run_synth_step }
    .disable_if(state.is_working() || state.examplar.empty()),
    trigger_button{ "Save output", &TextureSynthesis::save_image }
    .disable_if(state.is_working()),
    hstack {
      text( "Generated size" ),
      size_dial(1), 
      size_dial(0)
    },
    views::image{ state.examplar, refresh_examplar }.fit({300, 300}), 
    hstack { 
      views::image{ state.generated, refresh_display }.fit({600, 600}),
      views::image{ state.map, refresh_display, [sz = state.examplar.shape()] (auto& elem) {
        return rgb<float>{elem.m0.x / (float) sz.x, elem.m0.y / (float) sz.y, 0}; 
      }}.fit({600, 600})
    },
    /*hstack {
      // views::image { state.display },
      
      views::image{ state.examplar, refresh_examplar }, 
      views::image{ state.map, refresh_display, [sz = state.examplar.shape()] (auto& elem) {
        return rgb<float>{elem.m0.x / (float) sz.x, elem.m0.y / (float) sz.y, 0}; 
      }}, 
    },*/
    progress_bar { state.progress }
  };
}

void test_image() {
  image<vec2i> img {
    vec2i{720, 725}
  };
  
  for_each_pixel( img, [] (auto& pix, vec2i idx) {
    pix = idx;
  });
  
  vec2i margin = {17, 17};
  
  padded_image<vec2i> img2 { img, margin };
  
  for_each_pixel( img2, [&] (auto& p2, vec2i idx) {
    assert( p2 == idx + margin );
  });
  
  auto img3 = img;
  img3.reshape(img3.shape() - margin * 2);
  for_each_pixel( img3, [&] (auto& pix, vec2i idx) {
    pix = img(idx + margin);
  });
  
  assert( img2 == img3 );
}

inline void run_app() {
  test_image();
  TextureSynthesis state;
  auto app = make_app(state, &make_view);
  app.run(state);
}